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

#include <algorithm>
#include <string>

#include "eml_parser.h"

#include "attachment.h"
#include "exception.h"
#include "htmlcxx/html/CharsetConverter.h"
#include <iostream>
#include "log.h"
#include "metadata.h"
#include "plain_text_writer.h"
#include <pthread.h>
#include "mimetic/mimetic.h"
#include "mimetic/rfc822/rfc822.h"

using namespace mimetic;

const std::string attachment = "attachment";
const std::string multipart = "multipart/";
const std::string multipart_alternative = "multipart/alternative";
const std::string text = "text/";
const std::string text_html = "text/html";
const std::string text_xhtml = "text/xhtml";

static tm rfc_date_time_to_tm(const DateTime& date_time)
{
	tm tm_date_time;
	tm_date_time.tm_year = date_time.year() - 1900;
	tm_date_time.tm_mon = date_time.month().ordinal() - 1;
	tm_date_time.tm_mday = date_time.day();
	tm_date_time.tm_hour = date_time.hour();
	tm_date_time.tm_min = date_time.minute();
	tm_date_time.tm_sec = date_time.second();
	return tm_date_time;
}

pthread_mutex_t mimetic_content_id_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t charset_converter_mutex = PTHREAD_MUTEX_INITIALIZER;

struct EMLParser::Implementation
{
  EMLParser* m_owner;
	bool m_error;
	std::string m_file_name;
	std::istream* m_data_stream;
	std::vector<Link> m_links;
	std::vector<doctotext::Attachment> m_attachments;
  const std::shared_ptr<doctotext::ParserManager> m_parser_manager;

  Implementation(const std::shared_ptr<doctotext::ParserManager> &inParserManager, EMLParser* owner)
    : m_parser_manager(inParserManager),
      m_owner(owner)
  {}

	std::string getFilename(const MimeEntity& mime_entity)
	{
		if (mime_entity.hasField(ContentDisposition::label))
		{
			const ContentDisposition& cd = mime_entity.header().contentDisposition();
			if (cd.param("filename").length())
				return std::string(cd.param("filename").c_str());
		}
		else if (mime_entity.hasField(ContentType::label))
		{
			const ContentType& ct = mime_entity.header().contentType();
			return std::string("unnamed_" + ct.type() + "." + ct.subtype()).c_str();
		}
		return std::string("Unknown attachment");
	}

	void convertToUtf8(const std::string& charset, std::string& text)
	{
		try
		{
			pthread_mutex_lock(&charset_converter_mutex);
			htmlcxx::CharsetConverter converter(charset, "UTF-8");
			text = converter.convert(text);
			pthread_mutex_unlock(&charset_converter_mutex);
		}
		catch (htmlcxx::CharsetConverter::Exception& ex)
		{
			pthread_mutex_unlock(&charset_converter_mutex);
			doctotext_log(warning) << "Warning: Cant convert text to UTF-8 from " + charset;
		}
	}

	void decodeBin(const MimeEntity& me, std::string& out)
	{
		std::stringstream os(std::ios::out);
		std::ostreambuf_iterator<char> oi(os);
		const ContentTransferEncoding& cte = me.header().contentTransferEncoding();
		mimetic::istring enc_algo = cte.mechanism();
		if (enc_algo == ContentTransferEncoding::base64)
		{
			doctotext_log(debug) << "Using base64 decoding";
			Base64::Decoder b64;
			decode(me.body().begin(), me.body().end(), b64, oi);
		}
		else if (enc_algo == ContentTransferEncoding::quoted_printable)
		{
			doctotext_log(debug) << "Using quoted_printable decoding";
			QP::Decoder qp;
			decode(me.body().begin(), me.body().end(), qp, oi);
		}
		else if (enc_algo == ContentTransferEncoding::eightbit ||
			enc_algo == ContentTransferEncoding::sevenbit ||
			enc_algo == ContentTransferEncoding::binary)
		{
			doctotext_log(debug) << "Using eightbit/sevenbit/binary decoding";
			copy(me.body().begin(), me.body().end(), oi);
		}
		else
		{
			doctotext_log(debug) << "Unknown encoding: " + enc_algo;
			copy(me.body().begin(), me.body().end(), oi);
		}
		os.flush();
		out = os.rdbuf()->str();
	}

	MimeEntityList::const_iterator findHtmlNode(const MimeEntityList::const_iterator& begin, const MimeEntityList::const_iterator& end)
	{
	  MimeEntityList::const_iterator node_iterator;
	  for (node_iterator = begin; node_iterator != end; ++node_iterator)
	  {
	    const Header& header = (*node_iterator)->header();
	    if (header.contentType().str().substr(0, text_html.length()) == text_html ||
	        header.contentType().str().substr(0, text_xhtml.length()) == text_xhtml)
	      return node_iterator;
	  }
	  return node_iterator;
	}

  bool is_content_exist(const Header &header, const std::string &text)
  {
    pthread_mutex_lock(&mimetic_content_id_mutex);
    bool result = header.contentDisposition().str().find(attachment.c_str()) == std::string::npos &&
      header.contentType().str().substr(0, text.length()) == text;
    pthread_mutex_unlock(&mimetic_content_id_mutex);
    return result;
  }

	std::string parseText(const std::string &text, const std::string &type)
	{
		std::string parsed_text;
		PlainTextWriter writer;
		std::stringstream stream;
		auto callback = [&writer, &stream](const doctotext::Info &info){writer.write_to(info, stream);};
		auto parser_builder = m_parser_manager->findParserByExtension(type);
		if (parser_builder)
		{
			auto parser = (*parser_builder)->withParserManager(m_parser_manager)
							.withParameters(m_owner->m_parameters)
							.build(text.c_str(), text.length());
			parser->addOnNewNodeCallback(callback);
			parser->parse();
		}
		return stream.str();
	}

	void extractPlainText(const MimeEntity& mime_entity, std::string& output, const FormattingStyle& formatting)
	{
		const Header& header = mime_entity.header();
		if (is_content_exist(header, text))
		{
			std::string plain;
			decodeBin(mime_entity, plain);

			bool skip_charset_decoding = false;
			if (header.contentType().param("charset").length())
			{
				std::string charset(header.contentType().param("charset").c_str());
				convertToUtf8(charset, plain);
				skip_charset_decoding = true;
			}
			if (header.contentType().str().substr(0, text_html.length()) == text_html ||
					header.contentType().str().substr(0, text_xhtml.length()) == text_xhtml)
			{
				try
				{
					if (m_parser_manager)
					{
						plain = parseText(plain, "html");
					}
					//Update positions of the links.
					if (m_links.size() > 0)
					{
						size_t link_offset = output.length();
						for (size_t i = 0; i <m_links.size(); ++i)
							m_links[i].setLinkTextPosition(m_links[i].getLinkTextPosition() + link_offset);
					}
				}
				catch (Exception& ex)
				{
					doctotext_log(warning) << "Warning: Error while parsing html content";
				}
			}
			else
			{
				if (!skip_charset_decoding)
				{
					try
					{
						if (m_parser_manager)
						{
							plain = parseText(plain, "txt");
						}
					}
					catch (Exception& ex)
					{
						doctotext_log(warning) << "Warning: Error while parsing text content";
					}
				}
			}

      m_owner->sendTag(StandardTag::TAG_TEXT, plain + "\n\n");

			output += plain;
			output += "\n\n";
			return;
		}
		else if (header.contentType().str().substr(0, multipart.length()) != multipart)
		{
			std::string plain;
			decodeBin(mime_entity, plain);

      if (m_parser_manager)
      {
        std::string file_name = getFilename(mime_entity);
        std::string extension = file_name.substr(file_name.find_last_of(".") + 1);
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        auto info = m_owner->sendTag(
          StandardTag::TAG_ATTACHMENT, "", {{"name", file_name}, {"size", plain.length()}, {"extension", extension}});

        if(!info.skip)
        {
          auto parser_builder = m_parser_manager->findParserByExtension(file_name);

          auto parser = (*parser_builder)->withParserManager(m_parser_manager)
            .withOnNewNodeCallbacks({[this](doctotext::Info &info){m_owner->sendTag(info.tag_name, info.plain_text, info.attributes);}})
            .build(plain.c_str(), plain.length());
          parser->parse();
        }
        m_owner->sendTag(StandardTag::TAG_CLOSE_ATTACHMENT);
      }
			m_attachments.push_back(doctotext::Attachment(getFilename(mime_entity)));
			m_attachments[m_attachments.size() - 1].setBinaryContent(plain);
			std::string content_type = header.contentType().str();
			std::string content_disposition = header.contentDisposition().str();
			std::string content_description = header.contentDescription().str();
			pthread_mutex_lock(&mimetic_content_id_mutex);
			std::string content_id = header.contentId().str();
			pthread_mutex_unlock(&mimetic_content_id_mutex);
			if (!content_type.empty())
				m_attachments[m_attachments.size() - 1].addField("Content-Type", content_type);
			if (!content_disposition.empty())
				m_attachments[m_attachments.size() - 1].addField("Content-Disposition", content_disposition);
			if (!content_description.empty())
				m_attachments[m_attachments.size() - 1].addField("Content-Description", content_description);
			if (!content_id.empty())
				m_attachments[m_attachments.size() - 1].addField("Content-ID", content_id);
		}
		const MimeEntityList& parts = mime_entity.body().parts();
		MimeEntityList::const_iterator mbit = parts.begin(), meit = parts.end();

		if (header.contentType().str().substr(0, multipart_alternative.length()) == multipart_alternative)
		{
		  MimeEntityList::const_iterator html_node = findHtmlNode(mbit, meit);
		  if (html_node != meit)
		    extractPlainText(**html_node, output, formatting);
		  else if (mbit != meit)
		    extractPlainText(**mbit, output, formatting);
		}
		else
		{
		  for(; mbit != meit; ++mbit)
		    extractPlainText(**mbit, output, formatting);
		}
	}
};

EMLParser::EMLParser(const std::string& file_name, const std::shared_ptr<doctotext::ParserManager> &inParserManager)
  : Parser(inParserManager)
{
	impl = NULL;
	try
	{
		impl = new Implementation(inParserManager, this);
		impl->m_data_stream = NULL;
		impl->m_data_stream = new std::ifstream(file_name.c_str());
	}
	catch (std::bad_alloc& ba)
	{
		if (impl)
		{
			if (impl->m_data_stream)
				delete impl->m_data_stream;
			delete impl;
		}
		throw;
	}
}

EMLParser::EMLParser(const char* buffer, size_t size, const std::shared_ptr<doctotext::ParserManager> &inParserManager)
  : Parser(inParserManager)
{
	impl = NULL;
	try
	{
		impl = new Implementation(inParserManager, this);
		impl->m_data_stream = NULL;
		impl->m_data_stream = new std::stringstream(std::string(buffer, size));
	}
	catch (std::bad_alloc& ba)
	{
		if (impl)
		{
			if (impl->m_data_stream)
				delete impl->m_data_stream;
			delete impl;
		}
		throw;
	}
}

EMLParser::~EMLParser()
{
	if (impl)
	{
		if (impl->m_data_stream)
			delete impl->m_data_stream;
		delete impl;
	}
}

bool EMLParser::internal_is_eml() const
{
  MimeEntity mime_entity(*impl->m_data_stream);
  //"From" and "Date" are obligatory according to the RFC standard.
  Header header = mime_entity.header();
  return header.hasField("From") && header.hasField("Date");
}

bool EMLParser::isEML() const
{
	if (!impl->m_data_stream->good())
		throw Exception("Error opening file " + impl->m_file_name);
  pthread_mutex_lock(&mimetic_content_id_mutex);
  bool is_eml = internal_is_eml();
  pthread_mutex_unlock(&mimetic_content_id_mutex);
  return is_eml;
}

void EMLParser::getLinks(std::vector<Link>& links)
{
	links = impl->m_links;
}

void EMLParser::getAttachments(std::vector<doctotext::Attachment>& attachments)
{
	attachments = impl->m_attachments;
}

std::string EMLParser::plainText(const FormattingStyle& formatting) const
{
	std::string text;
	if (!isEML())
		throw Exception("Specified file is not valid EML file");
	impl->m_data_stream->seekg(0, std::ios_base::beg);
	MimeEntity mime_entity(*impl->m_data_stream);
	impl->extractPlainText(mime_entity, text, formatting);
	return text;
}

Metadata EMLParser::metaData()
{
	Metadata metadata;
  impl->m_data_stream->seekg(0, std::ios_base::beg);
	if (!isEML())
		throw Exception("Specified file is not valid EML file");
	impl->m_data_stream->seekg(0, std::ios_base::beg);
	MimeEntity mime_entity(*impl->m_data_stream);
	Header header = mime_entity.header();
	metadata.setAuthor(header.from().str());
	DateTime creation_date(header.field("Date").value());
	metadata.setCreationDate(rfc_date_time_to_tm(creation_date));

	//in EML file format author is visible under key "From". And creation date is visible under key "Data".
	//So, should I repeat the same values or skip them?
	metadata.addField("From", header.from().str());
	metadata.addField("Date", rfc_date_time_to_tm(creation_date));

	std::string to = header.to().str();
	if (!to.empty())
		metadata.addField("To", to);

	std::string subject = header.subject();
	if (!subject.empty())
		metadata.addField("Subject", subject);
	std::string mime_version = header.mimeVersion().str();
	if (!mime_version.empty())
		metadata.addField("MIME-Version", mime_version);
	std::string content_type = header.contentType().str();
	if (!content_type.empty())
		metadata.addField("Content-Type", content_type);
	std::string content_disposition = header.contentDisposition().str();
	if (!content_disposition.empty())
		metadata.addField("Content-Disposition", content_disposition);
	std::string content_description = header.contentDescription().str();
	if (!content_description.empty())
		metadata.addField("Content-Description", content_description);
	pthread_mutex_lock(&mimetic_content_id_mutex);
	std::string content_id = header.contentId().str();
	pthread_mutex_unlock(&mimetic_content_id_mutex);
	if (!content_id.empty())
		metadata.addField("Content-ID", content_id);
	std::string message_id = header.messageid().str();
	if (!message_id.empty())
		metadata.addField("Message-ID", message_id);
	std::string reply_to = header.replyto().str();
	if (!reply_to.empty())
		metadata.addField("Reply-To", reply_to);
	std::string sender = header.sender().str();
	if (!sender.empty())
		metadata.addField("Sender", sender);
	return metadata;
}

void
EMLParser::parse() const
{
	doctotext_log(debug) << "Using EML parser.";
  plainText(getFormattingStyle());
}
