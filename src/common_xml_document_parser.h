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

#ifndef DOCTOTEXT_COMMON_XML_PARSER_H
#define DOCTOTEXT_COMMON_XML_PARSER_H

#include "formatting_style.h"
#include "doctotext_link.h"
#include "parser.h"
#include <string>
#include <vector>
#include <map>

namespace doctotext
{
	class DocToTextUnzip;
	class Metadata;
	class XmlStream;
}

using namespace doctotext;


/**
	This class is inherited by ODFOOXMLParser and ODFXMLParser. It contains some common
	functions for both parsers.
	How inheritance works:
	Child classes (ODFOOXMLParser and ODFXMLParser for now) may want to add or change handlers for some xml tags
	(using registerODFOOXMLCommandHandler). Besides they have to implement two methods: plainText and metaData.
**/
class CommonXMLDocumentParser
{
	private:
		struct Implementation;
		Implementation* impl;
		class CommandHandlersSet;

	//public interface for derived classes (and its components)
	public:
		enum ODFOOXMLListStyle
		{
			number,
			bullet
		};

		struct Comment
		{
			std::string m_author;
			std::string m_time;
			std::string m_text;
			std::vector<Link> m_links;
			Comment() {}
			Comment(const std::string& author, const std::string& time, const std::string& text, const std::vector<Link>& links)
				: m_author(author), m_time(time), m_text(text), m_links(links) {}
		};

		struct SharedString
		{
			std::string m_text;
			std::vector<Link> m_links;
		};

		typedef std::vector<ODFOOXMLListStyle> ListStyleVector;

  typedef std::function<void(CommonXMLDocumentParser& parser, XmlStream& xml_stream, XmlParseMode mode,
                                 const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
                                 bool& children_processed, std::string& level_suffix, bool first_on_level,
                                 std::vector<Link>& links)> CommandHandler;

    void addCallback(const doctotext::NewNodeCallback &callback);

		/**
			Each xml tag can have associated handler, which is a single function of CommandHandler type.
			CommonXMLDocumentParser has already a set of functions for some basic tags.
			ODFOOXMLParser and ODFXMLParser can add new/overwrite existing handlers in order to change/extend default behaviour of
			parseXmlData/extractText.
		**/
		void registerODFOOXMLCommandHandler(const std::string& xml_tag, CommandHandler handler);

		///it is executed for each undefined tag (xml tag without associated handler). Can be overwritten
		virtual void onUnregisteredCommand(XmlStream& xml_stream, XmlParseMode mode,
										   const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text,
										   bool& children_processed, std::string& level_suffix, bool first_on_level,
										   std::vector<Link>& links);

		///parses xml data for given xml stream. It executes commands for each xml tag
		std::string parseXmlData(XmlStream& xml_stream, XmlParseMode mode, const FormattingStyle& options, const DocToTextUnzip* zipfile, std::vector<Link>& links) const; // todo https://github.com/docwire/doctotext/issues/91

		///extracts text and links from xml data. It uses parseXmlData internally. Throws doctotext::exception on fail
		void extractText(const std::string& xml_contents, XmlParseMode mode, const FormattingStyle& options, const DocToTextUnzip* zipfile, std::string& text, std::vector<Link>& links) const; // todo https://github.com/docwire/doctotext/issues/91

		///usefull since two parsers use this. Throws doctotext::exception on fail
		void parseODFMetadata(const std::string &xml_content, Metadata &metadata) const;

		///this is helpful function to format comment
		const std::string formatComment(const std::string& author, const std::string& time, const std::string& text);

		///Returns information "on how many list objects" we are. Returns 0 if we are not parsing any list actually. Should only be used inside command handlers
		size_t& getListDepth() const;

		///gets vector of links for reading and writing
		std::vector<Link>& getInnerLinks() const;

		///gets list styles for reading and writing
		std::map<std::string, ListStyleVector>& getListStyles() const;

		///gets comments for reading and writing
		std::map<int, Comment>& getComments() const;

		///gets vector of shared strings for reading and writing
		std::vector<SharedString>& getSharedStrings() const;

		///checks if writing to the text is disabled (only inside onUnregisteredCommand!)
		bool disabledText() const;

		///gets options which has been set for XmlStream object. (xmlParserOption from libxml2)
		int getXmlOptions() const;

		bool manageXmlParser() const;

		///disables modifying text data inside method onUnregisteredCommand
		void disableText(bool disable) const; // todo https://github.com/docwire/doctotext/issues/91

		///sets options for XmlStream objects. (xmlParserOption from libxml2)
		void setXmlOptions(int options) const; // todo https://github.com/docwire/doctotext/issues/91

		///Cleans up associated data. Call this method in constructor of derived class in case of bad_alloc.
		void cleanUp();

		void activeEmittingSignals(bool flag) const;

		void trySendTag(const std::string& tag_name, const std::string& text = "", const std::map<std::string, std::any> &attr = {}) const;

	//public interface
	public:
		CommonXMLDocumentParser();
		virtual ~CommonXMLDocumentParser();
		void setManageXmlParser(bool manage);
		void getLinks(std::vector<Link>& links);
		virtual std::string plainText(XmlParseMode mode, FormattingStyle& options) const = 0; // todo https://github.com/docwire/doctotext/issues/91
		virtual Metadata metaData() const = 0;
};

#endif
