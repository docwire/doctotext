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

#include "odfxml_parser.h"

#include "exception.h"
#include <fstream>
#include <iostream>
#include <boost/signals2.hpp>
#include <libxml/xmlreader.h>
#include "log.h"
#include "metadata.h"
#include "misc.h"
#include "xml_stream.h"

class ODFXMLParser::CommandHandlersSet
{
	public:
		static void onODFBody(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
							  const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
							  bool& children_processed, std::string& level_suffix, bool first_on_level,
							  std::vector<Link>& links)
		{
			// warning TODO: Unfortunately, in CommonXMLDocumentParser we are not checking full names for xml tags.\
			Thats a problem, since we can have table:body, office:body etc. What if more xml tags are not handled correctly?
			if (xml_stream.fullName() != "office:body")
				return;
			doctotext_log(debug) << "ODF_BODY Command";
			//we are inside body, we can disable adding text nodes
			parser.disableText(false);
		}

		static void onODFObject(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
								const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
								bool& children_processed, std::string& level_suffix, bool first_on_level,
								std::vector<Link>& links)
		{
			doctotext_log(debug) << "ODF_OBJECT Command";
			xml_stream.levelDown();
			parser.disableText(true);
			text += parser.parseXmlData(xml_stream, mode, options, zipfile, links);
			parser.disableText(false);
			xml_stream.levelUp();
		}

		static void onODFBinaryData(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
									const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
									bool& children_processed, std::string& level_suffix, bool first_on_level,
									std::vector<Link>& links)
		{
			doctotext_log(debug) << "ODF_BINARY_DATA Command";
			children_processed = true;
		}
};

struct ODFXMLParser::ExtendedImplementation
{
	const char* m_buffer;
	size_t m_buffer_size;
	std::string m_file_name;
	ODFXMLParser* m_interf;
  boost::signals2::signal<void(doctotext::Info &info)> m_on_new_node_signal;
};

ODFXMLParser::ODFXMLParser(const std::string& file_name, const std::shared_ptr<doctotext::ParserManager> &inParserManager)
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
		registerODFOOXMLCommandHandler("body", &CommandHandlersSet::onODFBody);
		registerODFOOXMLCommandHandler("object", &CommandHandlersSet::onODFObject);
		registerODFOOXMLCommandHandler("binary-data", &CommandHandlersSet::onODFBinaryData);
	}
	catch (std::bad_alloc& ba)
	{
		if (extended_impl)
			delete extended_impl;
		cleanUp();
		throw;
	}
}

ODFXMLParser::ODFXMLParser(const char *buffer, size_t size, const std::shared_ptr<doctotext::ParserManager> &inParserManager)
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
		registerODFOOXMLCommandHandler("body", &CommandHandlersSet::onODFBody);
		registerODFOOXMLCommandHandler("object", &CommandHandlersSet::onODFObject);
		registerODFOOXMLCommandHandler("binary-data", &CommandHandlersSet::onODFBinaryData);
	}
	catch (std::bad_alloc& ba)
	{
		if (extended_impl)
			delete extended_impl;
		cleanUp();
		throw;
	}
}

ODFXMLParser::~ODFXMLParser()
{
	if (extended_impl)
		delete extended_impl;
}

bool ODFXMLParser::isODFXML()
{
	std::string xml_content;
	if (extended_impl->m_buffer_size > 0)
		xml_content = std::string(extended_impl->m_buffer, extended_impl->m_buffer_size);
	else
	{
		std::ifstream file(extended_impl->m_file_name.c_str(), std::ios_base::in|std::ios_base::binary);
		if (!file.is_open())
			throw Exception("Error opening file: " + extended_impl->m_file_name);
		xml_content = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();
	}
	if (xml_content.find("office:document") == std::string::npos)
		return false;
	return true;
}

std::string ODFXMLParser::plainText(XmlParseMode mode, FormattingStyle& formatting_style) const
{
	std::string text;
	std::string xml_content;
	if (extended_impl->m_buffer_size > 0)
		xml_content = std::string(extended_impl->m_buffer, extended_impl->m_buffer_size);
	else
	{
		std::ifstream file(extended_impl->m_file_name.c_str(), std::ios_base::in|std::ios_base::binary);
		if (!file.is_open())
			throw Exception("Error opening file: " + extended_impl->m_file_name);
		xml_content = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();
	}

	//according to the ODF specification, we must skip blank nodes. Otherwise output from flat xml will be messed up.
	setXmlOptions(XML_PARSE_NOBLANKS);
	//in the beggining of xml stream, there are some options which we do not want to parse
	disableText(true);
	try
	{
		extractText(xml_content, mode, formatting_style, NULL, text, getInnerLinks());
	}
	catch (Exception& ex)
	{
		ex.appendError("Error parsing Flat XML file");
		throw;
	}
	decodeSpecialLinkBlocks(text, getInnerLinks());
	return text;
}

Metadata ODFXMLParser::metaData() const
{
	doctotext_log(debug) << "Extracting metadata.";
	Metadata metadata;

	std::string xml_content;
	if (extended_impl->m_buffer_size > 0)
		xml_content = std::string(extended_impl->m_buffer, extended_impl->m_buffer_size);
	else
	{
		std::ifstream file(extended_impl->m_file_name.c_str(), std::ios_base::in|std::ios_base::binary);
		if (!file.is_open())
			throw Exception("Error opening file: " + extended_impl->m_file_name);
		xml_content = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();
	}
	try
	{
		parseODFMetadata(xml_content, metadata);
	}
	catch (Exception& ex)
	{
		ex.appendError("Error parsing metadata in Flat XML file");
		throw;
	}
	if (metadata.pageCount() == -1)
	{
		// If we are processing ODP use slide count as page count
		// If we are processing ODG extract page count the same way
		if (xml_content.find("<office:presentation") != std::string::npos ||
			xml_content.find("<office:drawing") != std::string::npos)
		{
			int page_count = 0;
			std::string page_str = "<draw:page ";
			for (size_t pos = xml_content.find(page_str); pos != std::string::npos;
					pos = xml_content.find(page_str, pos + page_str.length()))
				page_count++;
			metadata.setPageCount(page_count);
		}
	}
	return metadata;
}

Parser&
ODFXMLParser::withParameters(const doctotext::ParserParameters &parameters)
{
	doctotext::Parser::withParameters(parameters);
	return *this;
}

void
ODFXMLParser::parse() const
{
	doctotext_log(debug) << "Using ODFXML parser.";
	auto formatting_style = getFormattingStyle();
  plainText(XmlParseMode::PARSE_XML, formatting_style);

  doctotext::Info info(StandardTag::TAG_METADATA, "", metaData().getFieldsAsAny());
  extended_impl->m_on_new_node_signal(info);
}

Parser&
ODFXMLParser::addOnNewNodeCallback(doctotext::NewNodeCallback callback)
{
  extended_impl->m_on_new_node_signal.connect(callback);
  CommonXMLDocumentParser::addCallback(callback);
  return *this;
}
