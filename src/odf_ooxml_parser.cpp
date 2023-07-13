/***************************************************************************************************************************************************/
/*  DocToText - A multifaceted, data extraction software development toolkit that converts all sorts of files to plain text and html.              */
/*  Written in C++, this data extraction tool has a parser able to convert PST & OST files along with a brand new API for better file processing.  */
/*  To enhance its utility, DocToText, as a data extraction tool, can be integrated with other data mining and data analytics applications.        */
/*  It comes equipped with a high grade, scriptable and trainable OCR that has LSTM neural networks based character recognition.                   */
/*                                                                                                                                                 */
/*  This document parser is able to extract metadata along with annotations and supports a list of formats that include:                           */
/*  DOC, XLS, XLSB, PPT, RTF, ODF (ODT, ODS, ODP), OOXML (DOCX, XLSX, PPTX), iWork (PAGES, NUMBERS, KEYNOTE), ODFXML (FODP, FODS, FODT),           */
/*  PDF, EML, HTML, Outlook (PST, OST), Image (JPG, JPEG, JFIF, BMP, PNM, PNG, TIFF, WEBP) and DICOM (DCM)                                         */
/*                                                                                                                                                 */
/*  Copyright (c) SILVERCODERS Ltd                                                                                                                 */
/*  http://silvercoders.com                                                                                                                        */
/*                                                                                                                                                 */
/*  Project homepage:                                                                                                                              */
/*  http://silvercoders.com/en/products/doctotext                                                                                                  */
/*  https://www.docwire.io/                                                                                                                        */
/*                                                                                                                                                 */
/*  The GNU General Public License version 2 as published by the Free Software Foundation and found in the file COPYING.GPL permits                */
/*  the distribution and/or modification of this application.                                                                                      */
/*                                                                                                                                                 */
/*  Please keep in mind that any attempt to circumvent the terms of the GNU General Public License by employing wrappers, pipelines,               */
/*  client/server protocols, etc. is illegal. You must purchase a commercial license if your program, which is distributed under a license         */
/*  other than the GNU General Public License version 2, directly or indirectly calls any portion of this code.                                    */
/*  Simply stop using the product if you disagree with this viewpoint.                                                                             */
/*                                                                                                                                                 */
/*  According to the terms of the license provided by SILVERCODERS and included in the file COPYING.COM, licensees in possession of                */
/*  a current commercial license for this product may use this file.                                                                               */
/*                                                                                                                                                 */
/*  This program is provided WITHOUT ANY WARRANTY, not even the implicit warranty of merchantability or fitness for a particular purpose.          */
/*  It is supplied in the hope that it will be useful.                                                                                             */
/***************************************************************************************************************************************************/

#include "odf_ooxml_parser.h"

#include "xml_fixer.h"
#include "doctotext_unzip.h"
#include "exception.h"
#include <fstream>
#include <iostream>
#include <libxml2/libxml/xmlreader.h>
#include "log.h"
#include <map>
#include "metadata.h"
#include "misc.h"
#include <regex>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <boost/signals2.hpp>
#include "xml_stream.h"
#include "thread_safe_ole_storage.h"
#include "thread_safe_ole_stream_reader.h"

using namespace std;

const int CASESENSITIVITY = 1;

static string locate_main_file(const DocToTextUnzip& zipfile)
{
	if (zipfile.exists("content.xml"))
		return "content.xml";
	if (zipfile.exists("word/document.xml"))
		return "word/document.xml";
	if (zipfile.exists("xl/workbook.xml"))
		return "xl/workbook.xml";
	if (zipfile.exists("ppt/presentation.xml"))
		return "ppt/presentation.xml";
	doctotext_log(error) << "Error - no content.xml, no word/document.xml and no ppt/presentation.xml";
	return "";
}

class ODFOOXMLParser::CommandHandlersSet
{
	public:
		static void onOOXMLAttribute(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
									 const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
									 bool& children_processed, std::string& level_suffix, bool first_on_level,
									 std::vector<Link>& links)
		{
			doctotext_log(debug) << "OOXML_ATTR command.";
			children_processed = true;
		}

    static void onOOXMLRow(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
                           const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
                           bool& children_processed, std::string& level_suffix, bool first_on_level,
                           std::vector<Link>& links)
    {
		ODFOOXMLParser& p = (ODFOOXMLParser&)parser;
		p.setLastOOXMLColNum(0);
		int expected_row_num = p.lastOOXMLRowNum() + 1;
		std::string row_num_attr = xml_stream.attribute("r");
		try
		{
			int row_num = std::stoi(row_num_attr);
			if (row_num > expected_row_num)
			{
				int empty_rows_count = row_num - expected_row_num;
				for (int i = 0; i < empty_rows_count; i++)
				{
					parser.trySendTag(StandardTag::TAG_TR);
					parser.trySendTag(StandardTag::TAG_CLOSE_TR);
				}
			}
			p.setLastOOXMLRowNum(row_num);
		}
		catch (std::invalid_argument const& ex)
		{
			// we accept when row id attribute is incorrect or missing
			p.setLastOOXMLRowNum(expected_row_num);
		}
      parser.trySendTag(StandardTag::TAG_TR);
      xml_stream.levelDown();
      text += parser.parseXmlData(xml_stream, mode, options, zipfile, links);
      xml_stream.levelUp();
      parser.trySendTag(StandardTag::TAG_CLOSE_TR);
    }

  static void onOOXMLSheetData(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
                         const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
                         bool& children_processed, std::string& level_suffix, bool first_on_level,
                         std::vector<Link>& links)
  {
	  ODFOOXMLParser& p = (ODFOOXMLParser&)parser;
	  p.setLastOOXMLRowNum(0);
    parser.trySendTag(StandardTag::TAG_TABLE);
    xml_stream.levelDown();
    text += parser.parseXmlData(xml_stream, mode, options, zipfile, links);
    xml_stream.levelUp();
    parser.trySendTag(StandardTag::TAG_CLOSE_TABLE);
  }

		static void onOOXMLCell(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
								const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
								bool& children_processed, std::string& level_suffix, bool first_on_level,
								std::vector<Link>& links)
		{
			doctotext_log(debug) << "OOXML_CELL command.";
			if (!first_on_level)
      {
				text += "\t";
      }
			ODFOOXMLParser& p = (ODFOOXMLParser&)parser;
			int expected_col_num = p.lastOOXMLColNum() + 1;
			std::string cell_addr_attr = xml_stream.attribute("r");
			std::regex cell_addr_rgx("([A-Z]+)([0-9]+)");
			std::smatch match;
			if (std::regex_match(cell_addr_attr, match, cell_addr_rgx) && match.size() == 3)
			{
				std::string col_addr = match[1].str();
				std::string row_addr = match[2].str();
				int col_num = 0;
				for (char ch: col_addr)
					col_num = col_num * 26 + (ch - 'A') + 1;
				int row_num = std::stoi(row_addr);
				if (col_num > expected_col_num)
				{
					int empty_cols_count = col_num - expected_col_num;
					for (int i = 0; i < empty_cols_count; i++)
					{
						parser.trySendTag(StandardTag::TAG_TD);
						parser.trySendTag(StandardTag::TAG_CLOSE_TD);
					}
				}
				p.setLastOOXMLColNum(col_num);
			}
			else
			{
				// we accept when cell address attribute is incorrect or missing
				p.setLastOOXMLRowNum(expected_col_num);
			}
      parser.trySendTag(StandardTag::TAG_TD);
			if (xml_stream.attribute("t") == "s")
			{
				xml_stream.levelDown();
        parser.activeEmittingSignals(false);
				int shared_string_index = str_to_int(parser.parseXmlData(xml_stream, mode, options, zipfile, links));
        parser.activeEmittingSignals(true);
				xml_stream.levelUp();
				if (shared_string_index < parser.getSharedStrings().size())
				{
					text += parser.getSharedStrings()[shared_string_index].m_text;
          parser.trySendTag(StandardTag::TAG_TEXT, parser.getSharedStrings()[shared_string_index].m_text);
					links.insert(links.begin(), parser.getSharedStrings()[shared_string_index].m_links.begin(), parser.getSharedStrings()[shared_string_index].m_links.end());
				}
			}
			else
			{
				xml_stream.levelDown();
        text += parser.parseXmlData(xml_stream, mode, options, zipfile, links);
				xml_stream.levelUp();
			}
      parser.trySendTag(StandardTag::TAG_CLOSE_TD);
			level_suffix = "\n";
			children_processed = true;
		}

		static void onOOXMLHeaderFooter(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
										const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
										bool& children_processed, std::string& level_suffix, bool first_on_level,
										std::vector<Link>& links)
		{
			doctotext_log(debug) << "OOXML_HEADERFOOTER command.";
			// Ignore headers and footers. They can contain some commands like font settings that can mess up output.
			// warning TODO: Better headers and footers support
			children_processed = true;
		}

		static void onOOXMLCommentReference(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
											const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
											bool& children_processed, std::string& level_suffix, bool first_on_level,
											std::vector<Link>& links)
		{
			doctotext_log(debug) << "OOXML_COMMENTREFERENCE command.";
			int comment_id = str_to_int(xml_stream.attribute("id"));
			if (parser.getComments().count(comment_id))
			{
				const Comment& c = parser.getComments()[comment_id];
				text += parser.formatComment(c.m_author, c.m_time, c.m_text);
				links.insert(links.end(), c.m_links.begin(), c.m_links.end());
				parser.trySendTag(StandardTag::TAG_COMMENT, "", {{"author",  c.m_author},
																													{"time",    c.m_time},
																													{"comment", c.m_text}});
			}
			else
				doctotext_log(warning) << "Comment with id " << comment_id << " not found, skipping.";
		}

		static void onOOXMLInstrtext(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
									 const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
									 bool& children_processed, std::string& level_suffix, bool first_on_level,
									 std::vector<Link>& links)
		{
			doctotext_log(debug) << "OOXML_INSTRTEXT command.";
			children_processed = true;
		}

		static void onOOXMLTableStyleId(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
									 const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
									 bool& children_processed, std::string& level_suffix, bool first_on_level,
									 std::vector<Link>& links)
		{
			doctotext_log(debug) << "OOXML_TABLESTYLEID command.";
			// Ignore style identifier that is embedded as text inside this tag not to treat it as a document text.
			children_processed = true;
		}
};

struct ODFOOXMLParser::ExtendedImplementation
{
	const char* m_buffer;
	size_t m_buffer_size;
	std::string m_file_name;
	int last_ooxml_col_num = 0;
	int last_ooxml_row_num = 0;
	ODFOOXMLParser* m_interf;
  boost::signals2::signal<void(doctotext::Info &info)> m_on_new_node_signal;

  void
  onOOXMLStyle(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
                               const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
                               bool& children_processed, std::string& level_suffix, bool first_on_level,
                               std::vector<Link>& links) const
  {
    xml_stream.levelDown();
    parser.activeEmittingSignals(false);
    text += parser.parseXmlData(xml_stream, mode, options, zipfile, links);
		xml_stream.levelUp();
    children_processed = true;
    parser.activeEmittingSignals(true);
  }

	void assertODFFileIsNotEncrypted(const DocToTextUnzip& zipfile)
	{
		std::string content;
		if (zipfile.exists("META-INF/manifest.xml")
			&& zipfile.read("META-INF/manifest.xml", &content)
			&& content.find("manifest:encryption-data") != std::string::npos)
			throw EncryptedFileException("Specified ODF file is encrypted");
	}

	bool fileIsEncrypted()
	{
		if (m_buffer)
			return is_encrypted_with_ms_offcrypto(m_buffer, m_buffer_size);
		else
			return is_encrypted_with_ms_offcrypto(m_file_name);
	}

	bool readOOXMLComments(const DocToTextUnzip& zipfile, XmlParseMode mode, FormattingStyle& options)
	{
		std::string content;
		if (!zipfile.read("word/comments.xml", &content))
		{
			doctotext_log(error) << "Error reading word/comments.xml";
			return false;
		}
		std::string xml;
		if (mode == FIX_XML)
		{
			DocToTextXmlFixer xml_fixer;
			xml = xml_fixer.fix(content);
		}
		else
			xml = content;
		try
		{
			XmlStream xml_stream(xml, m_interf->manageXmlParser(), m_interf->getXmlOptions());
			xml_stream.levelDown();
			while (xml_stream)
			{
				if (xml_stream.name() == "comment")
				{
					std::vector<Link> links;
					int id = str_to_int(xml_stream.attribute("id"));
					std::string author = xml_stream.attribute("author");
					std::string date = xml_stream.attribute("date");
					xml_stream.levelDown();
					m_interf->activeEmittingSignals(false);
					std::string text = m_interf->parseXmlData(xml_stream, mode, options, &zipfile, links);
					m_interf->activeEmittingSignals(true);
					xml_stream.levelUp();
					CommonXMLDocumentParser::Comment c(author, date, text, links);
					m_interf->getComments()[id] = c;
				}
				xml_stream.next();
			}
		}
		catch (Exception& ex)
		{
			doctotext_log(error) << "Error parsing word/comments.xml. Error message: " << ex.getBacktrace();
			return false;
		}
		return true;
	}

	void readStyles(const DocToTextUnzip& zipfile, XmlParseMode mode, FormattingStyle options)
	{
		std::string content;
		if (!zipfile.read("styles.xml", &content))
		{
			doctotext_log(error) << "Error reading styles.xml";
			return;
		}
		std::string xml;
		if (mode == FIX_XML)
		{
			DocToTextXmlFixer xml_fixer;
			xml = xml_fixer.fix(content);
		}
		else
			xml = content;
		try
		{
			XmlStream xml_stream(xml, m_interf->manageXmlParser(), m_interf->getXmlOptions());
			//we dont need those links (we dont expect them in "styles". But parseXmlData require this
			std::vector<Link> links;
			m_interf->parseXmlData(xml_stream, mode, options, &zipfile, links);
		}
		catch (Exception& ex)
		{
			doctotext_log(error) << "Error parsing styles.xml. Error message: " << ex.getBacktrace();
			return;
		}
	}
};

ODFOOXMLParser::ODFOOXMLParser(const string& file_name, const std::shared_ptr<doctotext::ParserManager> &inParserManager)
: Parser(inParserManager)
{
	extended_impl = NULL;
	try
	{
		extended_impl = new ExtendedImplementation();
		extended_impl->m_file_name = file_name;
		extended_impl->m_buffer = NULL;
		extended_impl->m_buffer_size = 0;
		extended_impl->m_interf = this;
		registerODFOOXMLCommandHandler("attrName", &CommandHandlersSet::onOOXMLAttribute);
		registerODFOOXMLCommandHandler("c", &CommandHandlersSet::onOOXMLCell);
    registerODFOOXMLCommandHandler("row", &CommandHandlersSet::onOOXMLRow);
    registerODFOOXMLCommandHandler("sheetData", &CommandHandlersSet::onOOXMLSheetData);
		registerODFOOXMLCommandHandler("headerFooter", &CommandHandlersSet::onOOXMLHeaderFooter);
		registerODFOOXMLCommandHandler("commentReference", &CommandHandlersSet::onOOXMLCommentReference);
		registerODFOOXMLCommandHandler("br", std::bind(
      &ODFOOXMLParser::onOOXMLBreak, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
      std::placeholders::_4,   std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, std::placeholders::_8,
      std::placeholders::_9, std::placeholders::_10));
    registerODFOOXMLCommandHandler("document-styles", std::bind(
      &ODFOOXMLParser::ExtendedImplementation::onOOXMLStyle, extended_impl, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
      std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, std::placeholders::_8,
      std::placeholders::_9, std::placeholders::_10));
		registerODFOOXMLCommandHandler("instrText", &CommandHandlersSet::onOOXMLInstrtext);
		registerODFOOXMLCommandHandler("tableStyleId", &CommandHandlersSet::onOOXMLTableStyleId);
	}
	catch (std::bad_alloc& ba)
	{
		if (extended_impl)
			delete extended_impl;
		cleanUp();
		throw;
	}
}

ODFOOXMLParser::ODFOOXMLParser(const char *buffer, size_t size, const std::shared_ptr<doctotext::ParserManager> &inParserManager)
: Parser(inParserManager)
{
	extended_impl = NULL;
	try
	{
		extended_impl = new ExtendedImplementation();
		extended_impl->m_file_name = "Memory buffer";
		extended_impl->m_buffer = buffer;
		extended_impl->m_buffer_size = size;
		extended_impl->m_interf = this;
		registerODFOOXMLCommandHandler("attrName", &CommandHandlersSet::onOOXMLAttribute);
		registerODFOOXMLCommandHandler("c", &CommandHandlersSet::onOOXMLCell);
		registerODFOOXMLCommandHandler("row", &CommandHandlersSet::onOOXMLRow);
    registerODFOOXMLCommandHandler("sheetData", &CommandHandlersSet::onOOXMLSheetData);
		registerODFOOXMLCommandHandler("headerFooter", &CommandHandlersSet::onOOXMLHeaderFooter);
		registerODFOOXMLCommandHandler("commentReference", &CommandHandlersSet::onOOXMLCommentReference);
		registerODFOOXMLCommandHandler("br", std::bind(
				&ODFOOXMLParser::onOOXMLBreak, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
				std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, std::placeholders::_8,
				std::placeholders::_9, std::placeholders::_10));
		registerODFOOXMLCommandHandler("document-styles", std::bind(
				&ODFOOXMLParser::ExtendedImplementation::onOOXMLStyle, extended_impl, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
				std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, std::placeholders::_8,
				std::placeholders::_9, std::placeholders::_10));
		registerODFOOXMLCommandHandler("instrText", &CommandHandlersSet::onOOXMLInstrtext);
		registerODFOOXMLCommandHandler("tableStyleId", &CommandHandlersSet::onOOXMLTableStyleId);
	}
	catch (std::bad_alloc& ba)
	{
		if (extended_impl)
			delete extended_impl;
		cleanUp();
		throw;
	}
}

ODFOOXMLParser::~ODFOOXMLParser()
{
	if (extended_impl)
		delete extended_impl;
}

Parser&
ODFOOXMLParser::withParameters(const doctotext::ParserParameters &parameters)
{
	doctotext::Parser::withParameters(parameters);
	return *this;
}

int ODFOOXMLParser::lastOOXMLRowNum()
{
	return extended_impl->last_ooxml_row_num;
}

void ODFOOXMLParser::setLastOOXMLRowNum(int r)
{
	extended_impl->last_ooxml_row_num = r;
}

int ODFOOXMLParser::lastOOXMLColNum()
{
	return extended_impl->last_ooxml_col_num;
}

void ODFOOXMLParser::setLastOOXMLColNum(int c)
{
	extended_impl->last_ooxml_col_num = c;
}

void
ODFOOXMLParser::onOOXMLBreak(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
                             const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
                             bool& children_processed, std::string& level_suffix, bool first_on_level,
                             std::vector<Link>& links) const
{
	doctotext_log(debug) << "OOXML_BREAK command.";
	text += "\n";

	parser.trySendTag(StandardTag::TAG_BR);
}

bool ODFOOXMLParser::isODFOOXML()
{
	if (!extended_impl->m_buffer)	//check file
	{
		FILE* f = fopen(extended_impl->m_file_name.c_str(), "r");
		if (f == NULL)
			throw Exception("Error opening file " + extended_impl->m_file_name);
		fclose(f);
	}
	else if (extended_impl->m_buffer_size == 0)
		throw Exception("Memory buffer is empty");
	DocToTextUnzip zipfile;
	if (extended_impl->m_buffer)
		zipfile.setBuffer(extended_impl->m_buffer, extended_impl->m_buffer_size);
	else
		zipfile.setArchiveFile(extended_impl->m_file_name);
	if (!zipfile.open())
	{
		if (extended_impl->fileIsEncrypted())
		{
			throw EncryptedFileException("File is encrypted according to the Microsoft Office Document Cryptography Specification. Exact file format cannot be determined");
		}
		return false;
	}
	string main_file_name = locate_main_file(zipfile);
	if (main_file_name == "")
	{
		zipfile.close();
		return false;
	}
	string contents;
	if (!zipfile.read(main_file_name, &contents, 1))
	{
		zipfile.close();
		return false;
	}
	zipfile.close();
	return true;
}

string ODFOOXMLParser::plainText(XmlParseMode mode, FormattingStyle& options) const
{
	DocToTextUnzip zipfile;
	if (extended_impl->m_buffer)
		zipfile.setBuffer(extended_impl->m_buffer, extended_impl->m_buffer_size);
	else
		zipfile.setArchiveFile(extended_impl->m_file_name);
	if (!zipfile.open())
	{
		if (extended_impl->fileIsEncrypted())
			throw EncryptedFileException("File is encrypted according to the Microsoft Office Document Cryptography Specification. Exact file format cannot be determined");
		throw Exception("Error opening file " + extended_impl->m_file_name + " as zip file");
	}
	string main_file_name = locate_main_file(zipfile);
	if (main_file_name == "")
		throw Exception("Could not locate main file inside zipped file (" + extended_impl->m_file_name + ")");
	//according to the ODF specification, we must skip blank nodes. Otherwise output may be messed up.
	if (main_file_name == "content.xml")
	{
		extended_impl->assertODFFileIsNotEncrypted(zipfile);
		setXmlOptions(XML_PARSE_NOBLANKS);
	}
	if (zipfile.exists("word/comments.xml") && !extended_impl->readOOXMLComments(zipfile, mode, options))
		doctotext_log(error) << "Error parsing comments.";
	if (zipfile.exists("styles.xml"))
		extended_impl->readStyles(zipfile, mode, options);
	string content;
	string text;
	if (main_file_name == "ppt/presentation.xml")
	{
		if (!zipfile.loadDirectory())
		{
			zipfile.close();
			throw Exception("Error loading zip directory of file " + extended_impl->m_file_name);
		}
		for (int i = 1; zipfile.read("ppt/slides/slide" + int_to_str(i) + ".xml", &content) && i < 2500; i++)
		{
			string slide_text;
			try
			{
				extractText(content, mode, options, &zipfile, slide_text, getInnerLinks());
			}
			catch (Exception& ex)
			{
				ex.appendError("Error while parsing file ppt/slides/slide" + int_to_str(i) + ".xml");
				zipfile.close();
				throw;
			}
			text += slide_text;
		}
	}
	else if (main_file_name == "xl/workbook.xml")
	{
		if (!zipfile.read("xl/sharedStrings.xml", &content))
		{
			//file may not exist, but this is not reason to report an error.
			doctotext_log(debug) << "xl/sharedStrings.xml does not exist";
		}
		else
		{
			std::string xml;
			if (mode == FIX_XML)
			{
				DocToTextXmlFixer xml_fixer;
				xml = xml_fixer.fix(content);
			}
			else if (mode == PARSE_XML)
				xml = content;
			else
			{
				zipfile.close();
				throw Exception("XML stripping not possible for xlsx format");
			}
			try
			{
				XmlStream xml_stream(xml, manageXmlParser(), getXmlOptions());
				xml_stream.levelDown();
				while (xml_stream)
				{
					if (xml_stream.name() == "si")
					{
						xml_stream.levelDown();
						SharedString shared_string;
            activeEmittingSignals(false);
						shared_string.m_text = parseXmlData(xml_stream, mode, options, &zipfile, shared_string.m_links);
            activeEmittingSignals(true);
						getSharedStrings().push_back(shared_string);
						xml_stream.levelUp();
					}
					xml_stream.next();
				}
			}
			catch (Exception& ex)
			{
				zipfile.close();
				throw Exception("Error parsing xl/sharedStrings.xml");
			}
		}
		for (int i = 1; zipfile.read("xl/worksheets/sheet" + int_to_str(i) + ".xml", &content); i++)
		{
			string sheet_text;
			try
			{
				extractText(content, mode, options, &zipfile, sheet_text, getInnerLinks());
			}
			catch (Exception& ex)
			{
				ex.appendError("Error while parsing file xl/worksheets/sheet" + int_to_str(i) + ".xml");
				zipfile.close();
				throw;
			}
			text += sheet_text;
		}
	}
	else
	{
		if (!zipfile.read(main_file_name, &content))
		{
			zipfile.close();
			throw Exception("Error reading xml contents of " + main_file_name);
		}
		try
		{
			extractText(content, mode, options, &zipfile, text, getInnerLinks());
		}
		catch (Exception& ex)
		{
			zipfile.close();
			ex.appendError("Error parsing xml contents of " + main_file_name);
			throw;
		}
	}
	decodeSpecialLinkBlocks(text, getInnerLinks());
	zipfile.close();
	return text;
}

Metadata ODFOOXMLParser::metaData() const
{
	doctotext_log(debug) << "Extracting metadata.";
	Metadata meta;
	DocToTextUnzip zipfile;
	if (extended_impl->m_buffer)
		zipfile.setBuffer(extended_impl->m_buffer, extended_impl->m_buffer_size);
	else
		zipfile.setArchiveFile(extended_impl->m_file_name);
	if (!zipfile.open())
	{
		if (extended_impl->fileIsEncrypted())
			throw EncryptedFileException("File is encrypted according to the Microsoft Office Document Cryptography Specification. Exact file format cannot be determined");
		throw Exception("Error opening file " + extended_impl->m_file_name + " as zip file");
	}
	if (zipfile.exists("meta.xml"))
	{
		extended_impl->assertODFFileIsNotEncrypted(zipfile);
		std::string meta_xml;
		if (!zipfile.read("meta.xml", &meta_xml))
		{
			zipfile.close();
			throw Exception("Error reading meta.xml");
		}
		try
		{
			parseODFMetadata(meta_xml, meta);
		}
		catch (Exception& ex)
		{
			ex.appendError("Error parsing meta.xml");
			zipfile.close();
			throw;
		}
	}
	else if (zipfile.exists("docProps/core.xml"))
	{
		std::string core_xml;
		if (!zipfile.read("docProps/core.xml", &core_xml))
		{
			zipfile.close();
			throw Exception("Error reading docProps/core.xml");
		}
		try
		{
			XmlStream xml_stream(core_xml, manageXmlParser(), getXmlOptions());
			xml_stream.levelDown();
			while (xml_stream)
			{
				if (xml_stream.name() == "creator")
					meta.setAuthor(xml_stream.stringValue());
				if (xml_stream.name() == "created")
				{
					tm creation_date;
					string_to_date(xml_stream.stringValue(), creation_date);
					meta.setCreationDate(creation_date);
				}
				if (xml_stream.name() == "lastModifiedBy")
					meta.setLastModifiedBy(xml_stream.stringValue());
				if (xml_stream.name() == "modified")
				{
					tm last_modification_date;
					string_to_date(xml_stream.stringValue(), last_modification_date);
					meta.setLastModificationDate(last_modification_date);
				}
				xml_stream.next();
			}
		}
		catch (Exception& ex)
		{
			zipfile.close();
			ex.appendError("Error parsing docProps/core.xml");
			throw;
		}

		std::string app_xml;
		if (!zipfile.read("docProps/app.xml", &app_xml))
		{
			zipfile.close();
			throw Exception("Error reading docProps/app.xml");
		}
		try
		{
			XmlStream app_stream(app_xml, manageXmlParser(), getXmlOptions());
			app_stream.levelDown();
			while (app_stream)
			{
				if (app_stream.name() == "Pages")
					meta.setPageCount(str_to_int(app_stream.stringValue()));
				if (app_stream.name() == "Words")
					meta.setWordCount(str_to_int(app_stream.stringValue()));
				app_stream.next();
			}
		}
		catch (Exception& ex)
		{
			zipfile.close();
			ex.appendError("Error parsing docProps/app.xml");
			throw;
		}
	}
	if (meta.pageCount() == -1)
	{
		// If we are processing PPT use slide count as page count
		if (zipfile.exists("ppt/presentation.xml"))
		{
			int page_count = 0;
			for (int i = 1; zipfile.exists("ppt/slides/slide" + int_to_str(i) + ".xml"); i++)
				page_count++;
			meta.setPageCount(page_count);
		}
		else if (zipfile.exists("content.xml"))
		{
			// If we are processing ODP use slide count as page count
			// If we are processing ODG extract page count the same way
			std::string content;
			if (zipfile.read("content.xml", &content) &&
				(content.find("<office:presentation") != std::string::npos ||
				content.find("<office:drawing") != std::string::npos))
			{
				int page_count = 0;
				std::string page_str = "<draw:page ";
				for (size_t pos = content.find(page_str); pos != std::string::npos;
						pos = content.find(page_str, pos + page_str.length()))
					page_count++;
				meta.setPageCount(page_count);
			}
		}
	}
	zipfile.close();
	return meta;
}

void
ODFOOXMLParser::parse() const
{
	doctotext_log(debug) << "Using ODF/OOXML parser.";
	auto formatting_style = getFormattingStyle();
	plainText(XmlParseMode::PARSE_XML, formatting_style);

	Metadata metadata = metaData();
	trySendTag(StandardTag::TAG_METADATA, "", metadata.getFieldsAsAny());
}

doctotext::Parser&
ODFOOXMLParser::addOnNewNodeCallback(doctotext::NewNodeCallback callback)
{
	CommonXMLDocumentParser::addCallback(callback);
	return *this;
}
