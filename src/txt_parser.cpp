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
#include "log.h"
#include <string.h>

using namespace doctotext;

struct TXTParser::Implementation
{
	std::string m_file_name;
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

namespace
{

std::string sequences_of_printable_characters(const std::string& text, size_t min_seq_len = 4, char seq_delim = '\n')
{
	std::string result;
	result.reserve(text.length());
	std::string printable_field;
	printable_field.reserve(text.length());
	size_t printable_count = 0;
	size_t non_printable_count = 0;
	for (auto const& ch: text)
	{
		if (std::isprint(ch))
		{
			printable_field += ch;
			printable_count++;
			non_printable_count = 0;
		}
		else
		{
			if (printable_count >= min_seq_len)
			{
				result += printable_field;
				if (non_printable_count == 0)
					result += seq_delim;
			}
			printable_field.clear();
			printable_count = 0;
			non_printable_count++;
		}
	}
	result += printable_field;
	return result;
}

} // anonymous namespace

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
			doctotext_log(warning) << "Warning: Could not create charset detector";
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
				doctotext_log(debug) << "Estimated encoding: " + encoding;
			}
			else
			{
				encoding = "ASCII";
				doctotext_log(debug) << "Could not detect encoding. Document is assumed to be encoded in ASCII";
				doctotext_log(debug) << "But it can be also binary. Sequences of printable characters will be extracted.";
				content = sequences_of_printable_characters(content);
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
				doctotext_log(warning) << "Warning: Cant convert text to UTF-8 from " + encoding;
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
		}
		else
			text = content;
	}
	catch (Exception& ex)
	{
		impl->m_data_stream->close();
		if (converter)
			delete converter;
		converter = NULL;
		if (charset_detector)
			csd_close(charset_detector);
		charset_detector = NULL;
		throw;
	}
	text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
	return text;
}

Parser&
TXTParser::withParameters(const doctotext::ParserParameters &parameters)
{
	doctotext::Parser::withParameters(parameters);
	return *this;
}

void
TXTParser::parse() const
{
	doctotext_log(debug) << "Using TXT parser.";

  Info info(StandardTag::TAG_TEXT, plainText());
  impl->m_on_new_node_signal(info);
}

TXTParser::Parser&
TXTParser::addOnNewNodeCallback(doctotext::NewNodeCallback callback)
{
  impl->m_on_new_node_signal.connect(callback);
  return *this;
}

std::vector <std::string> TXTParser::getExtensions()
{
	return
	{
		"asm", // Assembler source code
		"asp", // Active Server Page script page
		"aspx", // Active Server Page Extended ASP.NET script
		"bas", // Basic source code
		"bat", // Batch file (script)
		"c", // C source code
		"cc", // C++ language source code
		"cmake", // CMake module or script
		"cs", // Microsoft Visual Studio Visual C#.NET source code
		"conf", // Configuration information
		"cpp", // C++ source code file format
		"css", // Cascading Style Sheets
		"csv", // Comma Separated Value file
		"cxx", // C++ source code file format
		"d", // D Programming Language source code
		"f", "fpp", // Fortran source code
		"fs", // Microsoft Visual F# source code
		"go", // Google Go programming language source code
		"h", // C header data
		"hpp", // C++ header data
		"htm", "html", // HyperText Markup Language web page
		"hxx", // C++ header data
		"java", // Java language source code
		"js", // JavaScript source code script
		"json", // JavaScript object notation data interchange format
		"jsp", // JAVA Server page file
		"log", // Log
		"lua", // Lua script
		"md", // Markdown markup language source code
		"pas", // Delphi unit source code
		"php", // PHP script or page
		"pl", "perl", // Perl script language source code
		"py", // Python script language source code
		"r", // R script
		"rss", // Really Simple Syndication - RSS file format
		"sh", // Unix Bourne Shell (Bash) script
		"tcl", // TCL script source code
		"txt", "text", // Simple text
		"vb", "vbs", // Visual Basic script
		"xml", // XML document
		"xsd", // XML schema description
		"xsl", // XML eXtensible stylesheet
		"yml", "yaml", // YAML document
		"ws" // Microsoft Windows script
	};
}
