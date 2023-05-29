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

#include "txt_parser.h"

#include "charsetdetect.h"
#include "data_stream.h"
#include "exception.h"
#include "htmlcxx/html/CharsetConverter.h"
#include <boost/signals2.hpp>
#include <iostream>
#include <string.h>

using namespace doctotext;

struct TXTParser::Implementation
{
	std::string m_file_name;
	bool m_verbose_logging;
	std::ostream* m_log_stream;
	DataStream* m_data_stream;
  boost::signals2::signal<void(doctotext::Info &info)> m_on_new_node_signal;
};

TXTParser::TXTParser(const std::string& file_name, const std::shared_ptr<doctotext::ParserManager> &inParserManager)
: Parser(inParserManager)
{
	impl = NULL;
	try
	{
		impl = new Implementation;
		impl->m_data_stream = NULL;
		impl->m_data_stream = new FileStream(file_name);
		impl->m_verbose_logging = false;
		impl->m_log_stream = &std::cerr;
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

TXTParser::TXTParser(const char* buffer, size_t size, const std::shared_ptr<doctotext::ParserManager> &inParserManager)
: Parser(inParserManager)
{
	impl = NULL;
	try
	{
		impl = new Implementation;
		impl->m_data_stream = NULL;
		impl->m_data_stream = new BufferStream(buffer, size);
		impl->m_verbose_logging = false;
		impl->m_log_stream = &std::cerr;
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

TXTParser::~TXTParser()
{
	if (impl)
	{
		if (impl->m_data_stream)
			delete impl->m_data_stream;
		delete impl;
	}
}

void TXTParser::setVerboseLogging(bool verbose)
{
	impl->m_verbose_logging = verbose;
}

void TXTParser::setLogStream(std::ostream& log_stream)
{
	impl->m_log_stream = &log_stream;
}

std::string TXTParser::plainText() const
{
	std::string text;
	csd_t charset_detector = NULL;
	htmlcxx::CharsetConverter* converter = NULL;
	try
	{
		std::string encoding;
		if (!impl->m_data_stream->open())
			throw Exception("Could not open file: " + impl->m_file_name);
		std::string content;
		size_t file_size = impl->m_data_stream->size();
		content.resize(file_size);
		if (!impl->m_data_stream->read(&content[0], 1, file_size))
			throw Exception("Could not read from file: " + impl->m_file_name);
		impl->m_data_stream->close();
		charset_detector = csd_open();
		if (charset_detector == (csd_t)-1)
		{
			charset_detector = NULL;
			*impl->m_log_stream << "Warning: Could not create charset detector\n";
			encoding = "UTF-8";
		}
		else
		{
			csd_consider(charset_detector, content.c_str(), content.length());
			const char* res = csd_close(charset_detector);
			charset_detector = NULL;
			if (res != NULL)
			{
				encoding = std::string(res);
				if (impl->m_verbose_logging)
					*impl->m_log_stream << "Estimated encoding: " + encoding + "\n";
			}
			else
			{
				encoding = "ASCII";
				if (impl->m_verbose_logging)
					*impl->m_log_stream << "Could not detect encoding. Document is assumed to be encoded in ASCII\n";
			}
		}
		if (encoding != "utf-8" && encoding != "UTF-8")
		{
			try
			{
				converter = new htmlcxx::CharsetConverter(encoding, "UTF-8");
			}
			catch (htmlcxx::CharsetConverter::Exception& ex)
			{
				*impl->m_log_stream << "Warning: Cant convert text to UTF-8 from " + encoding;
				if (converter)
					delete converter;
				converter = NULL;
			}
		}
		if (converter)
		{
			text = converter->convert(content);
			delete converter;
			converter = NULL;
			return text;
		}
		return content;
	}
	catch (std::bad_alloc& ba)
	{
		if (converter)
			delete converter;
		converter = NULL;
		if (charset_detector)
			csd_close(charset_detector);
		charset_detector = NULL;
		throw;
	}
	catch (Exception& ex)
	{
		if (converter)
			delete converter;
		converter = NULL;
		if (charset_detector)
			csd_close(charset_detector);
		charset_detector = NULL;
		throw;
	}
	impl->m_data_stream->close();
	return text;
}

Parser&
TXTParser::withParameters(const doctotext::ParserParameters &parameters)
{
	doctotext::Parser::withParameters(parameters);
	impl->m_verbose_logging = isVerboseLogging();
	impl->m_log_stream = &getLogOutStream();
	return *this;
}

void
TXTParser::parse() const
{
	if (isVerboseLogging())
			getLogOutStream() << "Using TXT parser.\n";

  Info info(StandardTag::TAG_TEXT, plainText());
  impl->m_on_new_node_signal(info);
}

TXTParser::Parser&
TXTParser::addOnNewNodeCallback(doctotext::NewNodeCallback callback)
{
  impl->m_on_new_node_signal.connect(callback);
  return *this;
}
