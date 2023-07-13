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

#include "rtf_parser.h"

#include "data_stream.h"
#include "exception.h"
#include <iostream>
#include "log.h"
#include <map>
#include "metadata.h"
#include "misc.h"
#include "pthread.h"
#include <sstream>
#include <stack>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wv2/textconverter.h>
#include <wv2/ustring.h>
#include <boost/signals2.hpp>
using namespace wvWare;

struct RTFParser::Implementation
{
	std::string m_file_name;
	DataStream* m_data_stream;
  boost::signals2::signal<void(doctotext::Info &info)> m_on_new_node_signal;
};

RTFParser::RTFParser(const std::string& file_name, const std::shared_ptr<doctotext::ParserManager> &inParserManager)
: Parser(inParserManager)
{
	impl = NULL;
	try
	{
		impl = new Implementation();
		impl->m_file_name = file_name;
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

RTFParser::RTFParser(const char* buffer, size_t size, const std::shared_ptr<doctotext::ParserManager> &inParserManager)
: Parser(inParserManager)
{
	impl = NULL;
	try
	{
		impl = new Implementation();
		impl->m_file_name = "Memory buffer";
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

RTFParser::~RTFParser()
{
	if (impl)
	{
		if (impl->m_data_stream)
			delete impl->m_data_stream;
		delete impl;
	}
}

bool RTFParser::isRTF() const
{
	if (!impl->m_data_stream->open())
		throw Exception("Error opening file " + impl->m_file_name);
	char buf[6];
	if (!impl->m_data_stream->read(buf, sizeof(char), 5))
	{
		impl->m_data_stream->close();
		doctotext_log(error) << "Error reading signature from file " << impl->m_file_name << ".";
		return false;
	}
	impl->m_data_stream->close();
	buf[5] = '\0';
	return (strcmp(buf, "{\\rtf") == 0);
}

void RTFParser::getLinks(std::vector<Link>& links)
{
	// warning TODO: Implement this functionality.
}

#define RTFNAMEMAXLEN 32
#define RTFARGSMAXLEN 64

static long parseNumber(DataStream& data_stream)
{
	int ch;
	int count = 0;
	char buf[RTFARGSMAXLEN + 1];
	
	while (isdigit(ch = data_stream.getc()) || ch == '-')
	{
		if(data_stream.eof())
			return -1;
		if (count < RTFARGSMAXLEN)
			buf[count++] = (char)ch;
	}
	data_stream.unGetc(ch);
	buf[count] = '\0';
	return strtol(buf, (char **)NULL, 10);
}

int parseCharCode(DataStream& data_stream)
{
	int ch;
	int count = 0;
	char buf[RTFARGSMAXLEN + 1];

	for (int i = 0; i < 2; i++)
	{
		if (isdigit(ch = data_stream.getc()) || (ch >= 'a' && ch <= 'f'))
		{
			if(data_stream.eof())
				return -1;
			if (count < RTFARGSMAXLEN)
				buf[count++] = (char)ch;
		}
		else
			data_stream.unGetc(ch);
	}

	buf[count]='\0';
	return strtol(buf, (char **)NULL, 16);
}

enum RTFCommand
{
	RTF_CODEPAGE,
	RTF_FONT_CHARSET,
	RTF_UC,
	RTF_UNICODE_CHAR,
	RTF_CHAR,
	RTF_PARA,
	RTF_TABLE_START,
	RTF_TABLE_END,
	RTF_ROW,
	RTF_CELL,
	RTF_UNKNOWN,
	RTF_OVERLAY,
	RTF_PICT,
	RTF_F,
	RTF_AUTHOR,
	RTF_FONTTBL,
	RTF_INFO,
	RTF_STYLESHEET,
	RTF_COLORTBL,
	RTF_LISTOVERRIDETABLE,
	RTF_LISTTABLE,
	RTF_RSIDTBL,
	RTF_GENERATOR,
	RTF_DATAFIELD,
	RTF_LANG,
	RTF_LINE,
	RTF_PARD,
	RTF_TAB,
	RTF_SPEC_CHAR,
	RTF_EMDASH,
	RTF_ENDASH,
	RTF_EMSPACE,
	RTF_ENSPACE,
 	RTF_BULLET, 
 	RTF_LQUOTE,
	RTF_RQUOTE,
	RTF_LDBLQUOTE,
	RTF_RDBLQUOTE,
	RTF_ZWNONJOINER,
	RTF_FLDINST,
	RTF_FLDRSLT,
	RTF_ANNOTATION,
	RTF_ATNDATE,
	RTF_ATNAUTHOR
};

struct RTFStringCommand
{
	const char* name;
	RTFCommand cmd;
};

RTFStringCommand rtf_commands[] =
{
	{ "uc", RTF_UC},
	{ "ansicpg", RTF_CODEPAGE},
	{ "pard", RTF_PARD},
	{ "par", RTF_PARA},
	{ "cell", RTF_CELL},
	{ "row", RTF_ROW},
	{ "overlay", RTF_OVERLAY}, 
	{ "pict" ,RTF_PICT},
	{ "author", RTF_AUTHOR},
	{ "f", RTF_F}, 
	{ "fonttbl", RTF_FONTTBL}, 
	{ "info", RTF_INFO}, 
	{ "stylesheet", RTF_STYLESHEET},
	{ "colortbl", RTF_COLORTBL},
	{ "line", RTF_LINE},
	{ "listtable", RTF_LISTTABLE},
	{ "listoverridetable", RTF_LISTOVERRIDETABLE},
	{ "rsidtbl", RTF_RSIDTBL}, 
	{ "generator", RTF_GENERATOR}, 
	{ "datafield", RTF_DATAFIELD}, 
	{ "lang", RTF_LANG}, 
	{ "tab", RTF_TAB}, 
	{ "emdash", RTF_EMDASH},
	{ "endash", RTF_ENDASH},
	{ "emspace", RTF_EMDASH},
	{ "enspace", RTF_ENDASH},
	{ "bullet", RTF_BULLET}, 
	{ "lquote", RTF_LQUOTE},
	{ "rquote", RTF_RQUOTE},
	{ "ldblquote", RTF_LDBLQUOTE},
	{ "rdblquote", RTF_RDBLQUOTE},
	{ "zwnj", RTF_ZWNONJOINER},
	{ "u", RTF_UNICODE_CHAR},
	{ "fldinst", RTF_FLDINST},
	{ "fldrslt", RTF_FLDRSLT},
	{ "fcharset", RTF_FONT_CHARSET },
	{ "annotation", RTF_ANNOTATION },
	{ "atndate", RTF_ATNDATE },
	{ "atnauthor", RTF_ATNAUTHOR }
};

struct RTFGroup
{
	int uc;
	//int codepage;
	bool in_annotation;
};

struct RTFParserState
{
	std::stack<RTFGroup> groups;
	long int last_font_ref_num;
	std::map<long int, std::string> font_table;
	std::string author_of_next_annotation;
	tm annotation_time;
	UString annotation_text;
};

static RTFCommand commandNameToEnum(char* name)
{
	for (int i = 0; i < sizeof(rtf_commands) / sizeof(RTFStringCommand); i++)
	{
		if (strcmp(name, rtf_commands[i].name) == 0)
			return rtf_commands[i].cmd;
	}
	return RTF_UNKNOWN;
}

static bool parseCommand(DataStream& data_stream, RTFCommand& cmd, long int& arg)
{
	char name[RTFNAMEMAXLEN + 1];

	int ch = data_stream.getc();
	if (isalpha(ch))
	{
		int name_count = 1;
		name[0] = (char)ch;
		while (isalpha(ch = data_stream.getc()) && name_count < RTFNAMEMAXLEN)
		{
			if(data_stream.eof())
				return false;
			name[name_count++] = (char)ch;
		}
		name[name_count] = '\0';
		cmd = commandNameToEnum(name);
		data_stream.unGetc(ch);
		if (isdigit(ch) || ch == '-' )
			arg = parseNumber(data_stream);
		else
			arg = 0;
		ch = data_stream.getc();
		if(!(ch == ' ' || ch == '\t'))
			data_stream.unGetc(ch);
	}
	else
	{
		name[0] = (char)ch;
		name[1] = '\0';
		if (ch == '\'')
		{
			cmd = RTF_CHAR;
			arg = parseCharCode(data_stream);
			if(data_stream.eof())
				return false;
		}
		else
		{
			cmd = RTF_SPEC_CHAR;
			arg = ch;
		}
	}
	doctotext_log(debug) << "[cmd: " << name << " (" << arg << ")]";
	return true;
}

static std::string codepage_to_encoding(int codepage)
{
	char encoding[7];
	snprintf(encoding, 7, "CP%i", codepage);
	return encoding;
}

static std::string win_charset_to_encoding(long int win_charset)
{
	long int codepage = 1250;
	switch (win_charset)
	{
		case 0: codepage = 1250; break; // ANSI_CHARSET
		case 1: codepage = 1250; break; // DEFAULT_CHARSET
		//case 2: codepage = ; break; // SYMBOL_CHARSET not supported yet.
		case 77: codepage = 10000; break; // MAC_CHARSET
		case 128: codepage = 932; break; // SHIFTJIS_CHARSET
		case 129: codepage = 949; break; // HANGEUL_CHARSET
		case 130: codepage = 1361; break; // JOHAB_CHARSET
		case 134: codepage = 936; break; // GB2312_CHARSET
		case 136: codepage = 950; break; // CHINESEBIG5_CHARSET
		case 161: codepage = 1253; break; // GREEK_CHARSET
		case 162: codepage = 1254; break; // TURKISH_CHARSET
		case 163: codepage = 1258; break; // VIETNAMESE_CHARSET
		case 177: codepage = 1255; break; // HEBREW_CHARSET
		case 178: codepage = 1256; break; // ARABIC_CHARSET / ARABICSIMPLIFIED_CHARSET
		case 186: codepage = 1257; break; // BALTIC_CHARSET
		case 204: codepage = 1251; break; // RUSSIAN_CHARSET / CYRILLIC_CHARSET
		case 222: codepage = 874; break; // THAI_CHARSET
		case 238: codepage = 1250; break; // EASTEUROPE_CHARSET / 
		case 255: codepage = 850; break; // OEM_CHARSET
	}
	return codepage_to_encoding(codepage);
}

static void parse_dttm_time(int dttm, tm& tm)
{
	tm.tm_sec = 0;
	tm.tm_min = dttm & 0x0000003F;
	dttm >>= 6;
	tm.tm_hour = dttm & 0x0000001F;
	dttm >>= 5;
	tm.tm_mday = dttm & 0x0000001F;
	dttm >>= 5;
	tm.tm_mon = dttm & 0x0000000F - 1;
	dttm >>= 4;
	tm.tm_year = dttm & 0x000001FF;
}

static std::string format_comment(const std::string& author, const std::string& time, const std::string& text)
{
	std::string comment = "\n[[[COMMENT BY " + author + " (" + time + ")]]]\n" + text;
	if (text.empty() || *text.rbegin() != '\n')
		comment += "\n";
	comment += "[[[---]]]\n";
	return comment;
}

static void execCommand(DataStream& data_stream, UString& text, int& skip, RTFParserState& state, RTFCommand cmd, long int arg,
	TextConverter*& converter)
{
	switch (cmd)
	{
		case RTF_SPEC_CHAR:
			if (arg == '*' && skip == 0)
				skip = state.groups.size() - 1;
			else if (arg == '\r' || arg == '\n') // the same as \para command
				text += UString("\n");
			else if (arg == '~')
				text += UString((UChar)0xA0); /* No-break space */
			else if (arg == '-')
				text += UString((UChar)0xAD); /* Optional hyphen */
			break;
		case RTF_EMDASH:
			text += UString((UChar)0x2014);
			break;
		case RTF_ENDASH: 
			text += UString((UChar)0x2013);
			break;
		case RTF_BULLET: 
			text += UString((UChar)0x2022);
			break;
		case RTF_LQUOTE:
			text += UString((UChar)0x2018);
			break;
		case RTF_RQUOTE:
			text += UString((UChar)0x2019);
			break;
		case RTF_LDBLQUOTE:
			text += UString((UChar)0x201C);
			break;
		case RTF_RDBLQUOTE:
			text += UString((UChar)0x201D);
			break;
		case RTF_ZWNONJOINER:
			text += UString((UChar)0xfeff);
			break;
		case RTF_EMSPACE:
		case RTF_ENSPACE:
			text += UString(' ');
			break;
		case RTF_CHAR:
			if (skip == 0)
			{
				if (converter != NULL)
					text += converter->convert((const char*)&arg, sizeof(arg));
				else
					text += UString((UChar)arg);
			}
			break;
		case RTF_UC:
			state.groups.top().uc = arg;
			break;
		case RTF_TAB:
			text += UString((UChar)0x0009);
			break;
		case RTF_UNICODE_CHAR:
			if (arg < 0)
				break;
			if (skip == 0)
				text += UString((UChar)arg);
			for (int i = 0; i < state.groups.top().uc; i++)
			{
				char ch = data_stream.getc();
				if (ch == '\\')
				{
					RTFCommand tmp_cmd;
					long int tmp_arg;
					parseCommand(data_stream, tmp_cmd, tmp_arg);
				}
			}
			break;
		case RTF_PARA:
			text += UString("\n");
			break;
		case RTF_CELL:
			text += UString("\n");
			break;
		case RTF_FLDINST:
			text += UString("<");
			skip = 0;
			break;
		case RTF_FLDRSLT:
			text+=UString(">");
			break;
		case RTF_PICT:
		case RTF_FONTTBL:
		case RTF_INFO:
		case RTF_COLORTBL:
		case RTF_STYLESHEET:
		case RTF_LISTTABLE:
		case RTF_LISTOVERRIDETABLE:
		case RTF_RSIDTBL:
		case RTF_GENERATOR:
		case RTF_DATAFIELD:
			if (skip == 0)
				skip = state.groups.size() - 1;
			break;
		case RTF_LANG:
			break;
		case RTF_LINE:
			text += UString("\n");
			break;
		case RTF_CODEPAGE:
			doctotext_log(debug) << "Initializing converter for codepage " << arg;
			converter = new TextConverter(codepage_to_encoding(arg));
			if (converter->isOk())
			{
				doctotext_log(debug) << "Converter initialized.";
			}
			else
			{
				doctotext_log(error) << "Converter initialization ERROR!";
				delete converter;
				converter = NULL;
			}
			break;
		case RTF_FONT_CHARSET:
			doctotext_log(debug) << "Setting win charset " << arg << " for font number " << state.last_font_ref_num;
			state.font_table[state.last_font_ref_num] = win_charset_to_encoding(arg);
			break;
		case RTF_F:
			if (state.font_table.find(arg) != state.font_table.end())
			{
				doctotext_log(debug) << "Font number " << arg << " referenced. Setting converter for encoding " << state.font_table[arg];
				if (converter != NULL)
					converter->setFromCode(state.font_table[arg]);
			}
			else
				state.last_font_ref_num = arg;
			break;

		case RTF_ANNOTATION:
			state.groups.top().in_annotation = true;
			state.annotation_text = "";
			skip = 0;
			break;

		case RTF_ATNDATE:
		{
			char ch;
			std::string s;
			while (isdigit(ch = data_stream.getc()) && !data_stream.eof())
				s += ch;
			if (!data_stream.eof())
				data_stream.unGetc(ch);
			parse_dttm_time(str_to_int(s), state.annotation_time);
			break;
		}

		case RTF_ATNAUTHOR:
		{
			state.author_of_next_annotation = "";
			char ch;
			while ((ch = data_stream.getc()) != '}' && !data_stream.eof())
				state.author_of_next_annotation += ch;
			if (!data_stream.eof())
				data_stream.unGetc(ch);
			break;
		}
	}
}
static pthread_mutex_t converter_mutex = PTHREAD_MUTEX_INITIALIZER;
std::string RTFParser::plainText() const
{
	UString text;
	TextConverter* converter = NULL;
	try
	{
		if (!isRTF())	//check if this is really rtf file
			throw Exception("File " + impl->m_file_name + " is not rtf");
		if (!impl->m_data_stream->open())
			throw Exception("Error opening file " + impl->m_file_name);
		int ch;
		RTFParserState state;
		state.groups.push(RTFGroup());
		state.groups.top().uc = 1;
		state.groups.top().in_annotation = false;
		state.last_font_ref_num = 0;
		int skip = 0;
		while ((ch = impl->m_data_stream->getc()) != EOF)
		{
			switch (ch)
			{
				case '\\':
				{
					RTFCommand cmd;
					long int arg;
					if (!parseCommand(*impl->m_data_stream, cmd, arg))
						break;
					UString fragment_text;
					pthread_mutex_lock(&converter_mutex);
					execCommand(*impl->m_data_stream, fragment_text, skip, state, cmd, arg, converter);
					pthread_mutex_unlock(&converter_mutex);
					if (state.groups.top().in_annotation)
						state.annotation_text += fragment_text;
					else
						text += fragment_text;
					break;
				}

				case '{':
					state.groups.push(state.groups.top());
					state.groups.top().in_annotation = false;
					break;
				case '}':
				{
					bool in_annotation = state.groups.top().in_annotation;
					state.groups.pop();
					if (in_annotation && !state.groups.top().in_annotation)
						text += UString(format_comment(state.author_of_next_annotation, date_to_string(state.annotation_time), ustring_to_string(state.annotation_text)).c_str());
					if (skip > state.groups.size() - 1)
						skip = 0;
					break;
				}

				default:
					if (skip == 0 && (ch != '\n' || state.groups.top().in_annotation) && ch != '\r')
					{
						UString fragment_text;
						if (converter != NULL)
							fragment_text = converter->convert((const char*)&ch, sizeof(ch));
						else
							fragment_text = UString((UChar)ch);
						if (state.groups.top().in_annotation)
							state.annotation_text += fragment_text;
						else
							text += fragment_text;
					}
			}
			if (state.groups.size() == 0)	//it will crash soon if groups.size() returns zero... better to check
				throw Exception("File " + impl->m_file_name + " is corrupted");
		}
		impl->m_data_stream->close();
		if (converter != NULL)
			delete converter;
		converter = NULL;
		return ustring_to_string(text);
	}
	catch (std::bad_alloc& ba)
	{
		if (converter)
			delete converter;
		converter = NULL;
		throw;
	}
}

#include <fstream>
#include <sstream>

static void parse_rtf_time(const std::string& s, tm& time)
{
	time = tm();
	size_t p2 = s.find("\\yr");
	if (p2 != std::string::npos)
	{
		std::istringstream(s.substr(p2 + 3)) >> time.tm_year;
		if (time.tm_year == 0)
		{
			// Sometimes field exists but date is zero.
			// Last modification time saved by LibreOffice 3.5 when document is created is an example.
			return;
		}
		time.tm_year -= 1900;
	}
	p2 = s.find("\\mo");
	if (p2 != std::string::npos)
	{
		std::istringstream(s.substr(p2 + 3)) >> time.tm_mon;
		time.tm_mon--;
	}
	p2 = s.find("\\dy");
	if (p2 != std::string::npos)
		std::istringstream(s.substr(p2 + 3)) >> time.tm_mday;
	p2 = s.find("\\hr");
	if (p2 != std::string::npos)
		std::istringstream(s.substr(p2 + 3)) >> time.tm_hour;
	p2 = s.find("\\min");
	if (p2 != std::string::npos)
		std::istringstream(s.substr(p2 + 4)) >> time.tm_min;
	p2 = s.find("\\sec");
	if (p2 != std::string::npos)
		std::istringstream(s.substr(p2 + 4)) >> time.tm_sec;
}

Metadata RTFParser::metaData() const
{
	if (!isRTF())	//check if this is really rtf file
		throw Exception("File " + impl->m_file_name + " is not rtf");
	
	Metadata meta;
	doctotext_log(debug) << "Extracting metadata.";
	if (!impl->m_data_stream->open())
		throw Exception("Error opening file " + impl->m_file_name);
	size_t stream_size = impl->m_data_stream->size();
	std::vector<char> content_buffer(stream_size);
	if (!impl->m_data_stream->read(&content_buffer[0], 1, stream_size))
	{
		impl->m_data_stream->close();
		throw Exception("Error reading file " + impl->m_file_name);
	}
	std::string content(content_buffer.begin(), content_buffer.end());
	size_t p = content.find("\\author ");
	if (p != std::string::npos)
	{
		std::string author;
		for (int i = p + 8; content[i] != '{' && content[i] != '}'; i++)
			author += content[i];
		meta.setAuthor(author);
	}
	p = content.find("\\operator ");
	if (p != std::string::npos)
	{
		std::string last_modified_by;
		for (int i = p + 10; content[i] != '{' && content[i] != '}'; i++)
			last_modified_by += content[i];
		meta.setLastModifiedBy(last_modified_by);
	}
	p = content.find("\\creatim");
	if (p != std::string::npos)
	{
		std::string s;
		for (int i = p + 8; content[i] != '}'; i++)
			s += content[i];
		tm creation_date;
		parse_rtf_time(s, creation_date);
		meta.setCreationDate(creation_date);
	}
	p = content.find("\\revtim");
	if (p != std::string::npos)
	{
		std::string s;
		for (int i = p + 7; content[i] != '}'; i++)
			s += content[i];
		tm last_modification_date;
		parse_rtf_time(s, last_modification_date);
		meta.setLastModificationDate(last_modification_date);
	}
	p = content.find("\\nofpages");
	if (p != std::string::npos)
	{
		std::string s;
		for (int i = p + 9; content[i] != '}'; i++)
			s += content[i];
		int page_count;
		std::istringstream(s) >> page_count;
		meta.setPageCount(page_count);
	}
	p = content.find("\\nofwords");
	if (p != std::string::npos)
	{
		std::string s;
		for (int i = p + 9; content[i] != '}'; i++)
			s += content[i];
		int word_count;
		std::istringstream(s) >> word_count;
		meta.setWordCount(word_count);
	}
	impl->m_data_stream->close();
	return meta;
}

Parser&
RTFParser::withParameters(const doctotext::ParserParameters &parameters)
{
	doctotext::Parser::withParameters(parameters);
	return *this;
}

void
RTFParser::parse() const
{
	doctotext_log(debug) << "Using RTF parser.";
  Info info(StandardTag::TAG_TEXT, plainText());
  impl->m_on_new_node_signal(info);

  Metadata metadata = metaData();
  doctotext::Info metadata_info(StandardTag::TAG_METADATA, "", metadata.getFieldsAsAny());
  impl->m_on_new_node_signal(metadata_info);
}

doctotext::Parser&
RTFParser::addOnNewNodeCallback(doctotext::NewNodeCallback callback)
{
  impl->m_on_new_node_signal.connect(callback);
  return *this;
}
