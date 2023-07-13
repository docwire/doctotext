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

#include "common_xml_document_parser.h"

#include "doctotext_unzip.h"
#include "exception.h"
#include <iostream>
#include <libxml/xmlreader.h>
#include <functional>
#include "log.h"
#include "metadata.h"
#include "misc.h"
#include "xml_stream.h"
#include "xml_fixer.h"
#include <boost/signals2.hpp>

struct CommonXMLDocumentParser::Implementation
{
  template<typename T>
  CommandHandler
  add_command_handler(T member_function)
  {
    return std::bind(member_function, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                     std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, std::placeholders::_8,
                     std::placeholders::_9, std::placeholders::_10);
  }

  Implementation()
  : is_bold(false),
    is_italic(false),
    is_underline(false),
    space_preserve(false),
    stop_emmit_signals(false)
  {
    m_command_handlers["#text"] = add_command_handler<>(&Implementation::onODFOOXMLText);
    m_command_handlers["b"] = add_command_handler<>(&Implementation::onODFOOXMLBold);
    m_command_handlers["i"] = add_command_handler<>(&Implementation::onODFOOXMLItalic);
    m_command_handlers["u"] = add_command_handler<>(&Implementation::onODFOOXMLUnderline);
    m_command_handlers["p"] = add_command_handler<>(&Implementation::onODFOOXMLPara);
    m_command_handlers["rPr"] = add_command_handler<>(&Implementation::onrPr);
    m_command_handlers["pPr"] = add_command_handler<>(&Implementation::onpPr);
    m_command_handlers["r"] = add_command_handler<>(&Implementation::onR);
    m_command_handlers["tbl"] = add_command_handler<>(&Implementation::onODFOOXMLTable);
    m_command_handlers["tr"] = add_command_handler<>(&Implementation::onODFOOXMLTableRow);
    m_command_handlers["tc"] = add_command_handler<>(&Implementation::onODFOOXMLTableColumn);
    m_command_handlers["t"] = add_command_handler<>(&Implementation::onODFOOXMLTextTag);
  }

	bool m_manage_xml_parser;
	bool is_bold;
	bool is_italic;
	bool is_underline;
	bool space_preserve;
	bool stop_emmit_signals;
	size_t m_list_depth;
	std::map<std::string, ListStyleVector> m_list_styles;
	std::vector<Link> m_links;
	std::map<int, Comment> m_comments;
	std::vector<SharedString> m_shared_strings;
	std::map<std::string, CommandHandler> m_command_handlers;
	boost::signals2::signal<void(doctotext::Info &info)> m_on_new_node_signal;

	bool m_disabled_text;
	int m_xml_options;
	CommonXMLDocumentParser* m_parser;

	void send_tag(const std::string& tag_name, const std::string& text = "", const std::map<std::string, std::any> &attr = {})
	{
		if (!stop_emmit_signals)
		{
      doctotext::Info info(tag_name, text, attr);
			m_on_new_node_signal(info);
		}
	}

	void reset_format()
	{
		is_bold = false;
		is_italic = false;
		is_underline = false;
	}

  void onpPr(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
           const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
           bool& children_processed, std::string& level_suffix, bool first_on_level,
           std::vector<Link>& links)
  {
    xml_stream.levelDown();
    text += parser.parseXmlData(xml_stream, mode, options, zipfile, links);
    xml_stream.levelUp();
    children_processed = true;

    if (is_underline)
    {
      send_tag(StandardTag::TAG_CLOSE_U);
    }
    if (is_italic)
    {
      send_tag(StandardTag::TAG_CLOSE_I);
    }
    if (is_bold)
    {
      send_tag(StandardTag::TAG_CLOSE_B);
    }
    reset_format();
  }

	void onR(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
	           const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
	           bool& children_processed, std::string& level_suffix, bool first_on_level,
	           std::vector<Link>& links)
	{
		xml_stream.levelDown();
		text += parser.parseXmlData(xml_stream, mode, options, zipfile, links);
		xml_stream.levelUp();
		children_processed = true;

		if (is_underline)
		{
			send_tag(StandardTag::TAG_CLOSE_U);
		}
		if (is_italic)
		{
			send_tag(StandardTag::TAG_CLOSE_I);
		}
		if (is_bold)
		{
			send_tag(StandardTag::TAG_CLOSE_B);
		}
		reset_format();
	}

  void onrPr(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
                      const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
                      bool& children_processed, std::string& level_suffix, bool first_on_level,
                      std::vector<Link>& links)
  {
    reset_format();
    xml_stream.levelDown();
    parser.parseXmlData(xml_stream, mode, options, zipfile, links);
    if (is_bold)
    {
      send_tag(StandardTag::TAG_B);
    }
    if (is_italic)
    {
      send_tag(StandardTag::TAG_I);
    }
    if (is_underline)
    {
      send_tag(StandardTag::TAG_U);
    }
    xml_stream.levelUp();
    children_processed = true;
  }

  void onODFOOXMLPara(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
                      const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
                      bool& children_processed, std::string& level_suffix, bool first_on_level,
                      std::vector<Link>& links)
  {
    reset_format();
    send_tag(StandardTag::TAG_P);
    doctotext_log(debug) << "ODFOOXML_PARA command.";
    xml_stream.levelDown();
    text += parser.parseXmlData(xml_stream, mode, options, zipfile, links) + '\n';
    xml_stream.levelUp();
    children_processed = true;
    send_tag(StandardTag::TAG_CLOSE_P);
  }

  void onODFOOXMLText(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
                        const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
                        bool& children_processed, std::string& level_suffix, bool first_on_level,
                       std::vector<Link>& links)
  {
    if (m_disabled_text == false && xml_stream.name() == "#text")
    {
      char* content = xml_stream.content();
      if (content != NULL)
        text += content;
      children_processed = true;
      std::string s{content};
      if (space_preserve || !std::all_of(s.begin(), s.end(), [](auto c){return isspace(static_cast<unsigned char>(c));}))
        send_tag(StandardTag::TAG_TEXT, s);
    }
  }

  void onODFOOXMLTable(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
                      const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
                      bool& children_processed, std::string& level_suffix, bool first_on_level,
                      std::vector<Link>& links)
  {
    reset_format();
    send_tag(StandardTag::TAG_TABLE);
    doctotext_log(debug) << "onODFOOXMLTable command.";
    xml_stream.levelDown();
    text += parser.parseXmlData(xml_stream, mode, options, zipfile, links);
    xml_stream.levelUp();
    children_processed = true;
    send_tag(StandardTag::TAG_CLOSE_TABLE);
  }

  void onODFOOXMLTableRow(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
                       const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
                       bool& children_processed, std::string& level_suffix, bool first_on_level,
                       std::vector<Link>& links)
  {
    reset_format();
    send_tag(StandardTag::TAG_TR);
    doctotext_log(debug) << "onODFOOXMLTableRow command.";
    xml_stream.levelDown();
    text += parser.parseXmlData(xml_stream, mode, options, zipfile, links);
    xml_stream.levelUp();
    children_processed = true;
    send_tag(StandardTag::TAG_CLOSE_TR);
  }

  void onODFOOXMLTableColumn(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
                       const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
                       bool& children_processed, std::string& level_suffix, bool first_on_level,
                       std::vector<Link>& links)
  {
    reset_format();
    send_tag(StandardTag::TAG_TD);
    doctotext_log(debug) << "onODFOOXMLTableColumn command.";
    xml_stream.levelDown();
    text += parser.parseXmlData(xml_stream, mode, options, zipfile, links);
    xml_stream.levelUp();
    children_processed = true;
    send_tag(StandardTag::TAG_CLOSE_TD);
  }

  void onODFOOXMLTextTag(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
                      const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
                      bool& children_processed, std::string& level_suffix, bool first_on_level,
                      std::vector<Link>& links)
  {
    doctotext_log(debug) << "onODFOOXMLTextTag command.";
    bool space_preserve_prev = space_preserve;
    if (xml_stream.attribute("space") == "preserve")
      space_preserve = true;
    xml_stream.levelDown();
    text += parser.parseXmlData(xml_stream, mode, options, zipfile, links);
    xml_stream.levelUp();
    children_processed = true;
    space_preserve = space_preserve_prev;
  }

  void onODFOOXMLBold(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
                      const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
                      bool& children_processed, std::string& level_suffix, bool first_on_level,
                      std::vector<Link>& links)
  {
    is_bold = xml_stream.attribute("val") != "false";
  }

  void onODFOOXMLItalic(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
                      const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
                      bool& children_processed, std::string& level_suffix, bool first_on_level,
                      std::vector<Link>& links)
  {
    is_italic = xml_stream.attribute("val") != "false";
  }

  void onODFOOXMLUnderline(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
                      const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
                      bool& children_processed, std::string& level_suffix, bool first_on_level,
                      std::vector<Link>& links)
  {
    is_underline = xml_stream.attribute("val") != "none";
  }

	void executeCommand(const std::string& command, XmlStream& xml_stream, XmlParseMode mode,
						const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
						bool& children_processed, std::string& level_suffix, bool first_on_level,
						std::vector<Link>& links)
	{
		children_processed = false;
		std::map<std::string, CommonXMLDocumentParser::CommandHandler>::iterator it = m_command_handlers.find(command);
		if (it != m_command_handlers.end())
			it->second(*m_parser, xml_stream, mode, options, zipfile, text, children_processed, level_suffix, first_on_level, links);
		else
			m_parser->onUnregisteredCommand(xml_stream, mode, options, zipfile, text, children_processed, level_suffix, first_on_level, links);
	}
};

void
CommonXMLDocumentParser::trySendTag(const std::string& tag_name, const std::string& text, const std::map<std::string, std::any> &attr) const
{
  impl->send_tag(tag_name, text, attr);
}

void
CommonXMLDocumentParser::activeEmittingSignals(bool flag) const
{
  impl->stop_emmit_signals = !flag;
}

class CommonXMLDocumentParser::CommandHandlersSet
{
	public:
		static void onODFOOXMLText(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
								   const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
								   bool& children_processed, std::string& level_suffix, bool first_on_level,
								   std::vector<Link>& links)
		{
			doctotext_log(debug) << "ODFOOXML_TEXT command.";
			char* content = xml_stream.content();
			if (content != NULL)
				text += content;
		}

		static void onODFOOXMLTab(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
								  const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
								  bool& children_processed, std::string& level_suffix, bool first_on_level,
								  std::vector<Link>& links)
		{
			doctotext_log(debug) << "ODFOOXML_TAB command.";
			text += "\t";
			parser.impl->send_tag(StandardTag::TAG_TEXT, "\t", {});
		}

		static void onODFOOXMLSpace(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
								  const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
								  bool& children_processed, std::string& level_suffix, bool first_on_level,
								  std::vector<Link>& links)
		{
			doctotext_log(debug) << "ODFOOXML_SPACE command.";
			std::string count_attr = xml_stream.attribute("c");
			int count = 1;
			if (!count_attr.empty())
				count = str_to_int(count_attr);
			for (int i = 0; i < count; i++)
			{
				text += " ";
			}
			parser.impl->send_tag(StandardTag::TAG_TEXT, std::string(count, ' '));
		}

		static void onODFOOXMLUrl(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
								  const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
								  bool& children_processed, std::string& level_suffix, bool first_on_level,
								  std::vector<Link>& links)
		{
			doctotext_log(debug) << "ODFOOXML_URL command.";
			std::string mlink = xml_stream.attribute("href");
      parser.impl->send_tag(StandardTag::TAG_LINK, "", {{"url", mlink}});
			xml_stream.levelDown();
			std::string text_link = parser.parseXmlData(xml_stream, mode, options, zipfile, links);
			text_link = formatUrl(mlink, text_link, options);
			xml_stream.levelUp();
			Link link(mlink, text_link, 0);
			insertSpecialLinkBlockIntoText(text, link);
			links.push_back(link);
			children_processed = true;
      parser.impl->send_tag(StandardTag::TAG_CLOSE_LINK);
		}

		static void onODFOOXMLListStyle(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
										const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
										bool& children_processed, std::string& level_suffix, bool first_on_level,
										std::vector<Link>& links)
		{
			doctotext_log(debug) << "ODFOOXML_LIST_STYLE command.";
			std::string style_code = xml_stream.attribute("name");
			if (!style_code.empty())
			{
				CommonXMLDocumentParser::ListStyleVector list_style(10, CommonXMLDocumentParser::bullet);
				xml_stream.levelDown();
				while (xml_stream)
				{
					std::string list_style_name = xml_stream.name();
					unsigned int level = strtol(xml_stream.attribute("level").c_str(), NULL, 10);
					if (level <= 10 && level > 0)
					{
						if (list_style_name == "list-level-style-number")
							list_style[level - 1] = CommonXMLDocumentParser::number;
						else
							list_style[level - 1] = CommonXMLDocumentParser::bullet;
					}
					xml_stream.next();
				}
				xml_stream.levelUp();
				children_processed = true;
				parser.getListStyles()[style_code] = list_style;
			}
		}

		static void onODFOOXMLList(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
								   const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
								   bool& children_processed, std::string& level_suffix, bool first_on_level,
								   std::vector<Link>& links)
		{
			std::vector<std::string> list_vector;
			doctotext_log(debug) << "ODFOOXML_LIST command.";
			++parser.getListDepth();
			std::string header;
			CommonXMLDocumentParser::ODFOOXMLListStyle list_style = CommonXMLDocumentParser::bullet;
			std::string style_name = xml_stream.attribute("style-name");
			if (parser.getListDepth() <= 10 && !style_name.empty() && parser.getListStyles().find(style_name) != parser.getListStyles().end())
				list_style = parser.getListStyles()[style_name].at(parser.getListDepth() - 1);
			parser.impl->send_tag(StandardTag::TAG_LIST, "", { { "is_ordered", list_style == CommonXMLDocumentParser::number }, {"list_style_prefix", std::string(options.list_style.getPrefix())} });

			xml_stream.levelDown();
			while (xml_stream)
			{
				std::string list_element_type = xml_stream.name();
				xml_stream.levelDown();
				if (list_element_type == "list-header" && xml_stream)
				{
					header = parser.parseXmlData(xml_stream, mode, options, zipfile, links);
				}
				else if (xml_stream)
				{
					parser.impl->send_tag(StandardTag::TAG_LIST_ITEM);
					list_vector.push_back(parser.parseXmlData(xml_stream, mode, options, zipfile, links));
				}

				parser.impl->send_tag(StandardTag::TAG_CLOSE_LIST_ITEM);

				xml_stream.levelUp();
				xml_stream.next();
			}
			xml_stream.levelUp();

			if (header.length() > 0)
			{
				text += header;
				if (list_vector.size() > 0)
					text += "\n";
					parser.impl->send_tag(StandardTag::TAG_BR);
			}
			--parser.getListDepth();
			parser.impl->send_tag(StandardTag::TAG_CLOSE_LIST);
			if (list_style == CommonXMLDocumentParser::number)
				text += formatNumberedList(list_vector);
			else
				text += formatList(list_vector, options);
			children_processed = true;
		}

		static void onODFOOXMLTable(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
									const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
									bool& children_processed, std::string& level_suffix, bool first_on_level,
									std::vector<Link>& links)
		{
			svector cell_vector;
			std::vector<svector> row_vector;
			doctotext_log(debug) << "ODFOOXML_TABLE command.";
			parser.impl->send_tag(StandardTag::TAG_TABLE);
			xml_stream.levelDown();
			while (xml_stream)
			{
				if (xml_stream.name() == "table-row")
				{
					xml_stream.levelDown();
					cell_vector.clear();
					parser.impl->send_tag(StandardTag::TAG_TR);
					while (xml_stream)
					{
						if (xml_stream.name() == "table-cell")
						{
							parser.impl->send_tag(StandardTag::TAG_TD);
							xml_stream.levelDown();
							cell_vector.push_back(parser.parseXmlData(xml_stream, mode, options, zipfile, links));
							xml_stream.levelUp();
							parser.impl->send_tag(StandardTag::TAG_CLOSE_TD);
						}
						xml_stream.next();
					}
					row_vector.push_back(cell_vector);
					xml_stream.levelUp();
					parser.impl->send_tag(StandardTag::TAG_CLOSE_TR);
				}
				xml_stream.next();
			}
			xml_stream.levelUp();
			parser.impl->send_tag(StandardTag::TAG_CLOSE_TABLE);
			text += formatTable(row_vector, options);
			children_processed = true;
		}

		static void onODFOOXMLTableRow(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
									   const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
									   bool& children_processed, std::string& level_suffix, bool first_on_level,
									   std::vector<Link>& links)
		{
			doctotext_log(debug) << "ODFOOXML_ROW command.";
		}

		static void onODFTableCell(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
								   const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
								   bool& children_processed, std::string& level_suffix, bool first_on_level,
								   std::vector<Link>& links)
		{
			doctotext_log(debug) << "ODF_CELL command.";
		}

		static void onODFAnnotation(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
									const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
									bool& children_processed, std::string& level_suffix, bool first_on_level,
									std::vector<Link>& links)
		{
			doctotext_log(debug) << "ODF_ANNOTATION command.";
			std::string creator;
			std::string date;
			std::string content;
			xml_stream.levelDown();
			while (xml_stream)
			{
				if (xml_stream.name() == "creator")
					creator = xml_stream.stringValue();
				if (xml_stream.name() == "date")
					date = xml_stream.stringValue();
				if (xml_stream.name() == "p")
				{
					if (!content.empty())
						content += "\n";
					xml_stream.levelDown();
					parser.activeEmittingSignals(false);
					content += parser.parseXmlData(xml_stream, mode, options, zipfile, links);
					parser.activeEmittingSignals(true);
					xml_stream.levelUp();
				}
				xml_stream.next();
			}
			xml_stream.levelUp();
      parser.trySendTag(StandardTag::TAG_COMMENT, "", {{"author",  creator},
                                                       {"time",    date},
                                                       {"comment", content}});
			text += parser.formatComment(creator, date, content);
			children_processed = true;
		}

		static void onODFLineBreak(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
								   const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
								   bool& children_processed, std::string& level_suffix, bool first_on_level,
								   std::vector<Link>& links)
		{
			doctotext_log(debug) << "ODF_LINE_BREAK command.";
			text += "\n";
			parser.impl->send_tag(StandardTag::TAG_BR);
		}

		static void onODFHeading(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
								 const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
								 bool& children_processed, std::string& level_suffix, bool first_on_level,
								 std::vector<Link>& links)
		{
			doctotext_log(debug) << "ODF_HEADING command.";
			xml_stream.levelDown();
			text += parser.parseXmlData(xml_stream, mode, options, zipfile, links) + '\n';
			xml_stream.levelUp();
			children_processed = true;
		}

		static void onODFObject(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
								const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
								bool& children_processed, std::string& level_suffix, bool first_on_level,
								std::vector<Link>& links)
		{
			doctotext_log(debug) << "ODF_OBJECT command.";
			std::string href = xml_stream.attribute("href");
			std::string content_fn = (href.substr(0, 2) == "./" ? href.substr(2) : href) + "/content.xml";
			std::string content;
			if (!zipfile->read(content_fn, &content))
			{
				doctotext_log(error) << "Error reading " << content_fn;
				return;
			}
			std::string object_text;
			try
			{
				parser.extractText(content, mode, options, zipfile, object_text, links);
			}
			catch (Exception& ex)
			{
				doctotext_log(error) << "Error parsing " << content_fn;
			}
			text += object_text;
		}
};

void CommonXMLDocumentParser::registerODFOOXMLCommandHandler(const std::string& xml_tag, CommandHandler handler)
{
	impl->m_command_handlers[xml_tag] = handler;
}

void CommonXMLDocumentParser::onUnregisteredCommand(XmlStream& xml_stream, XmlParseMode mode,
													const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
													bool& children_processed, std::string& level_suffix, bool first_on_level,
													std::vector<Link>& links)
{
	if (impl->m_disabled_text == false && xml_stream.name() == "#text")
	{
		char* content = xml_stream.content();
		if (content != NULL)
			text += content;
		children_processed = true;
	}
}

void
CommonXMLDocumentParser::addCallback(const doctotext::NewNodeCallback &callback)
{
  impl->m_on_new_node_signal.connect(callback);
}

std::string CommonXMLDocumentParser::parseXmlData(
  XmlStream& xml_stream, XmlParseMode mode, const FormattingStyle& options, const DocToTextUnzip* zipfile, std::vector<Link>& links) const
{
	std::string text;
	std::string level_suffix;
	bool first_on_level = true;

	while (xml_stream)
	{
		bool children_processed;
		impl->executeCommand(xml_stream.name(), xml_stream, mode, options, zipfile, text, children_processed, level_suffix, first_on_level, links);
		if (xml_stream && (!children_processed))
		{
			xml_stream.levelDown();
			if (xml_stream)
				text += parseXmlData(xml_stream, mode, options, zipfile, links);
			xml_stream.levelUp();
		}
		xml_stream.next();
		first_on_level = false;
	}
	if (!level_suffix.empty())
		text += level_suffix;
	return text;
}

void CommonXMLDocumentParser::extractText(const std::string& xml_contents, XmlParseMode mode, const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text, std::vector<Link>& links) const
{
	if (mode == STRIP_XML)
	{
		text = "";
		bool in_tag = false;
		for (int i = 0; i < xml_contents.length(); i++)
		{
			char ch = xml_contents[i];
			if (ch == '<')
				in_tag = true;
			else if (ch == '>')
				in_tag = false;
			else if (!in_tag)
				text += ch;
		}
		return;
	}
	std::string xml;
	if (mode == FIX_XML)
	{
		DocToTextXmlFixer xml_fixer;
		xml = xml_fixer.fix(xml_contents);
	}
	else
		 xml = xml_contents;
	try
	{
		XmlStream xml_stream(xml, impl->m_manage_xml_parser, getXmlOptions());
		text = parseXmlData(xml_stream, mode, options, zipfile, links);
	}
	catch (Exception& ex)
	{
		ex.appendError("Error parsing XML contents");
		throw;
	}
}

void CommonXMLDocumentParser::parseODFMetadata(const std::string &xml_content, Metadata &metadata) const
{
	try
	{
		XmlStream xml_stream(xml_content, impl->m_manage_xml_parser, XML_PARSE_NOBLANKS);
		xml_stream.levelDown();
		while (xml_stream)
		{
			if (xml_stream.name() == "meta")
			{
				xml_stream.levelDown();
				while (xml_stream)
				{
					if (xml_stream.name() == "initial-creator")
						metadata.setAuthor(xml_stream.stringValue());
					if (xml_stream.name() == "creation-date")
					{
						tm creation_date;
						string_to_date(xml_stream.stringValue(), creation_date);
						metadata.setCreationDate(creation_date);
					}
					if (xml_stream.name() == "creator")
						metadata.setLastModifiedBy(xml_stream.stringValue());
					if (xml_stream.name() == "date")
					{
						tm last_modification_date;
						string_to_date(xml_stream.stringValue(), last_modification_date);
						metadata.setLastModificationDate(last_modification_date);
					}
					if (xml_stream.name() == "document-statistic")
					{
						std::string attr = xml_stream.attribute("meta::page-count"); // LibreOffice 3.5
						if (attr.empty())
							attr = xml_stream.attribute("page-count"); // older OpenOffice.org
						if (!attr.empty())
							metadata.setPageCount(str_to_int(attr));
						attr = xml_stream.attribute("meta:word-count"); // LibreOffice 3.5
						if (attr.empty())
							attr = xml_stream.attribute("word-count"); // older OpenOffice.org
						if (!attr.empty())
							metadata.setWordCount(str_to_int(attr));
					}
					xml_stream.next();
				}
				xml_stream.levelUp();
			}
			xml_stream.next();
		}
	}
	catch (Exception& ex)
	{
		ex.appendError("Error while parsing ODF metadata");
		throw;
	}
}

const std::string CommonXMLDocumentParser::formatComment(const std::string& author, const std::string& time, const std::string& text)
{
	std::string comment = "\n[[[COMMENT BY " + author + " (" + time + ")]]]\n" + text;
	if (text.empty() || *text.rbegin() != '\n')
		comment += "\n";
	comment += "[[[---]]]\n";
	return comment;
}

size_t& CommonXMLDocumentParser::getListDepth() const
{
	return impl->m_list_depth;
}

std::vector<Link>& CommonXMLDocumentParser::getInnerLinks() const
{
	return impl->m_links;
}

std::map<std::string, CommonXMLDocumentParser::ListStyleVector>& CommonXMLDocumentParser::getListStyles() const
{
	return impl->m_list_styles;
}

std::map<int, CommonXMLDocumentParser::Comment>& CommonXMLDocumentParser::getComments() const
{
	return impl->m_comments;
}

std::vector<CommonXMLDocumentParser::SharedString>& CommonXMLDocumentParser::getSharedStrings() const
{
	return impl->m_shared_strings;
}

bool CommonXMLDocumentParser::disabledText() const
{
	return impl->m_disabled_text;
}

bool CommonXMLDocumentParser::manageXmlParser() const
{
	return impl->m_manage_xml_parser;
}

void CommonXMLDocumentParser::disableText(bool disable) const
{
	impl->m_disabled_text = disable;
}

void CommonXMLDocumentParser::setXmlOptions(int options) const
{
	impl->m_xml_options = options;
}

void CommonXMLDocumentParser::cleanUp()
{
	if (impl)
		delete impl;
}

CommonXMLDocumentParser::CommonXMLDocumentParser()
{
	impl = NULL;
	try
	{
		impl = new Implementation;
		impl->m_list_depth = 0;
		impl->m_manage_xml_parser = true;
		impl->m_disabled_text = false;
		impl->m_xml_options = 0;
		impl->m_parser = this;

		registerODFOOXMLCommandHandler("text", &CommandHandlersSet::onODFOOXMLText);
		registerODFOOXMLCommandHandler("tab", &CommandHandlersSet::onODFOOXMLTab);
		registerODFOOXMLCommandHandler("space", &CommandHandlersSet::onODFOOXMLSpace);
		registerODFOOXMLCommandHandler("s", &CommandHandlersSet::onODFOOXMLSpace);
		registerODFOOXMLCommandHandler("a", &CommandHandlersSet::onODFOOXMLUrl);
		registerODFOOXMLCommandHandler("list-style", &CommandHandlersSet::onODFOOXMLListStyle);
		registerODFOOXMLCommandHandler("list", &CommandHandlersSet::onODFOOXMLList);
		registerODFOOXMLCommandHandler("table", &CommandHandlersSet::onODFOOXMLTable);
		registerODFOOXMLCommandHandler("table-row", &CommandHandlersSet::onODFOOXMLTableRow);
		registerODFOOXMLCommandHandler("table-cell", &CommandHandlersSet::onODFTableCell);
		registerODFOOXMLCommandHandler("annotation", &CommandHandlersSet::onODFAnnotation);
		registerODFOOXMLCommandHandler("line-break", &CommandHandlersSet::onODFLineBreak);
		registerODFOOXMLCommandHandler("h", &CommandHandlersSet::onODFHeading);
		registerODFOOXMLCommandHandler("object", &CommandHandlersSet::onODFObject);
	}
	catch (std::bad_alloc& ba)
	{
		if (impl)
			delete impl;
		impl = NULL;
		throw;
	}
}

CommonXMLDocumentParser::~CommonXMLDocumentParser()
{
	cleanUp();
}

void CommonXMLDocumentParser::setManageXmlParser(bool manage)
{
	impl->m_manage_xml_parser = manage;
}

int CommonXMLDocumentParser::getXmlOptions() const
{
	return impl->m_xml_options;
}

void CommonXMLDocumentParser::getLinks(std::vector<Link>& links)
{
	links = impl->m_links;
}
