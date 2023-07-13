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

#include "doc_parser.h"
#include <iostream>
#include "log.h"
#include "misc.h"

//Dirty hack, be we now what we are doing and this is internal library.
#define protected public
#define private public
#include "wv2/parser.h"
#include "wv2/parser9x.h"
#undef protected
#undef private

#include "exception.h"
#include "wv2/fields.h"
#include "wv2/handlers.h"
#include "metadata.h"
#include "oshared.h"
#include "wv2/paragraphproperties.h"
#include "wv2/parserfactory.h"
#include <stdio.h>
#include <sstream>
#include "wv2/ustring.h"
#include <vector>
#include "pthread.h"
#ifdef WIN32
	#include <windows.h>
#endif
#include <boost/signals2.hpp>
#include "wv2/word_helper.h"
#include "wv2/word97_generated.h"
#include "wv2/wvlog.h"
#include "xls_parser.h"
#include "thread_safe_ole_stream_reader.h"
#include "thread_safe_ole_storage.h"

using namespace wvWare;

static pthread_mutex_t parser_factory_mutex_1 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t parser_factory_mutex_2 = PTHREAD_MUTEX_INITIALIZER;

struct CurrentTable
{
	bool in_table;
	std::string curr_cell_text;
	svector curr_row_cells;
	std::vector<svector> rows;
	CurrentTable() : in_table(false) {};
};

struct CurrentHeaderFooter
{
	bool in_header;
	bool in_footer;
	UString header;
	UString footer;
	CurrentHeaderFooter() : in_header(false), in_footer(false) {};
};

// constants are from MS Word Binary File Format Specification
enum FieldType
{
	FLT_NONE = 0,
	FLT_FILLIN = 0x27,
	FLT_EMBED = 0x3A,
	FLT_HYPERLINK = 0x58
};

enum FieldPart
{
	FIELD_PART_NONE,
	FIELD_PART_PARAMS,
	FIELD_PART_VALUE
};

struct CurrentState
{
	CurrentTable table;
	CurrentHeaderFooter header_footer;
	std::list<std::string> obj_texts;
	std::list<std::string>::iterator obj_texts_iter;
	FieldType field_type;
	FieldPart field_part;
	UString field_params;
	UString field_value;
	CurrentState() : field_type(FLT_NONE), field_part(FIELD_PART_NONE) {};
};

struct Comment
{
	int fc;
	std::string text;
	std::string author;
};

static bool cp_to_stream_offset(const wvWare::Parser* parser, U32 cp, U32& stream_offset, bool* unicode_detected = NULL)
{
	const Parser9x* parser9 = dynamic_cast<const Parser9x*>(parser);
	if (parser9 == NULL)
	{
		doctotext_log(warning) << "This is not a 9x parser.";
		return false;
	}
	U32 piece = 0;
	U32 offset = cp;
	if (parser9->m_plcfpcd == NULL)
	{
		doctotext_log(warning) << "No pieces table found.";
		return false;
	}
	// Copied from wv2 code.
	PLCFIterator<Word97::PCD> it( *parser9->m_plcfpcd );
	for ( ; it.current(); ++it, ++piece ) {
		if ( it.currentLim() > cp && it.currentStart() <= cp )
		break;
		offset -= it.currentRun();
	}
	doctotext_log(debug) << "Piece: " << piece << ", offset: " << offset;
	PLCFIterator<Word97::PCD> it2(parser9->m_plcfpcd->at( piece ) );
	if (!it2.current())
	{
		doctotext_log(warning) << "Specified piece not found.";
		return false;
	}
	U32 fc = it2.current()->fc;   // Start FC of this piece
	doctotext_log(debug) << "Piece start at FC " << fc << ".";
        bool unicode;
        parser9->realFC(fc, unicode);
	doctotext_log(debug) << "After unicode transition piece start at FC " << fc << ".";
	if (offset != 0 )
            fc += unicode ? offset * 2 : offset;
	doctotext_log(debug) << "Stream offset is " << fc << ".";
	stream_offset = fc;
	if (unicode_detected != NULL)
		*unicode_detected = unicode;
	return true;
}

static bool parse_comments(const wvWare::Parser* parser, std::vector<Comment>& comments)
{
	AbstractOLEStreamReader* reader = NULL;
	AbstractOLEStreamReader* table_reader = NULL;
	try
	{
		if (parser->fib().lcbPlcfandTxt == 0)
		{
			doctotext_log(debug) << "No annotations.";
			return true;
		}

		U32 atn_part_cp = parser->fib().ccpText + parser->fib().ccpFtn + parser->fib().ccpHdd + parser->fib().ccpMcr;
		doctotext_log(debug) << "Annotations part at CP " << atn_part_cp << ".";
		reader = parser->m_storage->createStreamReader("WordDocument");
		if (!reader)
		{
			doctotext_log(error) << "Error opening WordDocument stream.";
			return false;
		}
		const Parser9x* parser9 = dynamic_cast<const Parser9x*>(parser);
		table_reader = parser->m_storage->createStreamReader(parser9->tableStream());
		if (!table_reader)
		{
			doctotext_log(error) << "Error opening table stream.";
			delete reader;
			return false;
		}

		U32 annotation_txts_offset = parser->fib().fcPlcfandTxt;
		doctotext_log(debug) << "Annotation texts table at offset " << annotation_txts_offset << ".";
		table_reader->seek(annotation_txts_offset);
		U32 annotation_begin_cp = table_reader->readU32();
		std::vector<std::string> annotations;
		for (;;)
		{
			U32 annotation_end_cp = table_reader->readU32();
			doctotext_log(debug) << "Annotation text position (CP) from " << annotation_begin_cp << " to " << annotation_end_cp << ".";
			U32 stream_begin_offset;
			bool unicode;
			if (!cp_to_stream_offset(parser, atn_part_cp + annotation_begin_cp, stream_begin_offset, &unicode))
			{
				doctotext_log(error) << "Converting annotation start position to stream offset failed.";
				delete reader;
				delete table_reader;
				return false;
			}
			U32 stream_end_offset;
			if (!cp_to_stream_offset(parser, atn_part_cp + annotation_end_cp, stream_end_offset))
			{
				doctotext_log(error) << "Converting annotation end position to stream offset failed.";
				delete reader;
				delete table_reader;
				return false;
			}
			doctotext_log(debug) << "Annotation text stream position from " << stream_begin_offset << " to " << stream_end_offset << ".";
			reader->seek(stream_begin_offset);
			U8 annotation_mark = reader->readU8();
			if (!annotation_mark == 0x05)
			{
				doctotext_log(error) << "Incorrect annotation mark.";
				delete reader;
				delete table_reader;
				return false;
			}
			std::string annotation;
			while (reader->tell() < stream_end_offset - 1)
			{
				doctotext_log(debug) << "Stream pos " << reader->tell();
				// warning TODO: Unicode support in comments
				if (unicode)
					reader->seek(1, SEEK_CUR); // skip unicode byte
				S8 ch = reader->readS8();
				if (ch >= 32 || (ch >= 8 && ch <= 13))
				{
					if (ch == '\r')
						annotation += "\n";
					else
						annotation += ch;
				}
			}
			doctotext_log(debug) << "Annotation text: \"" << annotation << "\"";
			annotations.push_back(annotation);
			if (annotation_end_cp >= parser->fib().ccpAtn - 1)
				break;
			annotation_begin_cp = annotation_end_cp;
		}

		U32 annotation_owners_offset = parser->fib().fcGrpXstAtnOwners;
		U32 annotation_owners_len = parser->fib().lcbGrpXstAtnOwners;
		doctotext_log(debug) << "Annotation owners table at offset " << annotation_owners_offset << " has length " << annotation_owners_len << ".";
		table_reader->seek(annotation_owners_offset);
		std::vector<std::string> owners;
		for (int total_len = 0; total_len < annotation_owners_len;)
		{
			U16 len = table_reader->readU16();
			total_len += 2;
			std::string owner;
			if (len * 2 + total_len > annotation_owners_len)
			{
				doctotext_log(warning) << "Something is wrong with XST table. Authors of comments could not be parsed.";
				owners.clear();
				break;
			}
			for (int i = 0; i < len; i++)
			{
				S16 ch = table_reader->readS16();
				owner += (char)ch;
				total_len += 2;
			}
			doctotext_log(debug) << "Owner \"" << owner << "\" found.";
			owners.push_back(owner);
		}

		U32 annotation_refs_offset = parser->fib().fcPlcfandRef;
		doctotext_log(debug) << "Annotation refs table at offset " << annotation_refs_offset << ".";
		table_reader->seek(annotation_refs_offset);
		for (int i = 0;; i++)
		{
			U32 annotation_ref_cp = table_reader->readU32();
			if (annotation_ref_cp >= parser->fib().ccpText)
				break;
			doctotext_log(debug) << "Annotation " << i << " references text at CP " << annotation_ref_cp << ".";
			U32 stream_offset;
			if (!cp_to_stream_offset(parser, annotation_ref_cp, stream_offset))
			{
				doctotext_log(error) << "Converting annotation reference position to stream offset failed.";
				delete reader;
				delete table_reader;
				return false;
			}
			if (i < annotations.size())
			{
				doctotext_log(debug) << "Annotation " << i << " references text at stream offset " << stream_offset << ".";
				Comment c;
				c.fc = stream_offset;
				c.text = annotations[i];
				comments.push_back(c);
			}
		}
		table_reader->seek(annotation_refs_offset + 4 * (comments.size() + 1));
		for (int i = 0; i < comments.size(); i++)
		{
			Word97::ATRD atrd(table_reader);
			if (atrd.ibst < owners.size())
			{
				doctotext_log(debug) << "Annotation owner is \"" << owners[atrd.ibst] << "\", index " << atrd.ibst << ".";
				comments[i].author = owners[atrd.ibst];
			}
		}
		delete table_reader;
		delete reader;
		return true;
	}
	catch (std::bad_alloc& ba)
	{
		if (table_reader)
			delete table_reader;
		if (reader)
			delete reader;
		throw;
	}
}

static std::string format_comment(const std::string& author, const std::string& text)
{
	std::string comment = "\n[[[COMMENT BY " + author + "]]]\n" + text;
	if (text.empty() || *text.rbegin() != '\n')
		comment += "\n";
	comment += "[[[---]]]\n";
	return comment;
}

class DocToTextTextHandler : public TextHandler
{
	private:
		const wvWare::Parser* m_parser;
		UString* Text;
		CurrentState* m_curr_state;
		FormattingStyle m_formatting;
		bool m_comments_parsed;
		std::vector<Comment> m_comments;
		U32 m_prev_par_fc;

	public:
		DocToTextTextHandler(const wvWare::Parser* parser, UString* text, CurrentState* curr_state,
				const FormattingStyle& formatting)
			: m_parser(parser), m_curr_state(curr_state), m_comments_parsed(false), m_prev_par_fc(0)
		{
			Text = text;
			m_curr_state = curr_state;
			m_formatting = formatting;
		}

		void sectionStart(SharedPtr<const Word97::SEP> sep)
		{
		}

		void sectionEnd()
		{
		}

		void pageBreak()
		{
		}

		void paragraphStart(SharedPtr<const ParagraphProperties>
			paragraphProperties)
		{
			if (((Parser9x*)m_parser)->m_currentParagraph->size() > 0)
			{
				if (m_comments_parsed)
				{
					for (int i = 0; i < m_comments.size(); i++)
					if (m_comments[i].fc >= m_prev_par_fc && m_comments[i].fc < ((Parser9x*)m_parser)->m_currentParagraph->back().m_startFC)
						(*Text) += utf8_to_ustring(format_comment(m_comments[i].author, m_comments[i].text));
				}
				else
				{
					if (!parse_comments(m_parser, m_comments))
					{
						doctotext_log(error) << "Parsing comments failed.";
						m_comments.clear();
					}
					m_comments_parsed = true;
				}
			}
			if (m_curr_state->table.in_table && (!paragraphProperties->pap().fInTable))
			{
				m_curr_state->table.in_table = false;
				(*Text) += utf8_to_ustring(formatTable(m_curr_state->table.rows, m_formatting));
				m_curr_state->table.rows.clear();
			}
		}

		void paragraphEnd()
		{
			if (m_curr_state->table.in_table)
				m_curr_state->table.curr_cell_text += "\n";
			else if (m_curr_state->header_footer.in_header)
				m_curr_state->header_footer.header += UString("\n");
			else if (m_curr_state->header_footer.in_footer)
				m_curr_state->header_footer.footer += UString("\n");
			else
				(*Text) += UString("\n");
			if (!((Parser9x*)m_parser)->m_currentParagraph->empty())
			{
				m_prev_par_fc = ((Parser9x*)m_parser)->m_currentParagraph->back().m_startFC;
			}
		}

		void runOfText (const UString &text, SharedPtr< const Word97::CHP > chp)
		{
			if (m_curr_state->field_part == FIELD_PART_PARAMS)
				m_curr_state->field_params += text;
			else if (m_curr_state->field_part == FIELD_PART_VALUE)
				m_curr_state->field_value += text;
			else if (m_curr_state->table.in_table)
				m_curr_state->table.curr_cell_text += ustring_to_string(text);
			else if (m_curr_state->header_footer.in_header)
				m_curr_state->header_footer.header += text;
			else if (m_curr_state->header_footer.in_footer)
				m_curr_state->header_footer.footer += text;
			else
				(*Text) += text;
		}

		void specialCharacter(SpecialCharacter character,
			SharedPtr<const Word97::CHP> chp)
		{
		}

		void footnoteFound (FootnoteData::Type type, UChar character,
			SharedPtr<const Word97::CHP> chp,
			const FootnoteFunctor &parseFootnote)
		{
		}

		void footnoteAutoNumber(SharedPtr<const Word97::CHP> chp)
		{
		}

		void fieldStart(const FLD *fld,
			SharedPtr<const Word97::CHP> chp)
		{
			m_curr_state->field_type = (FieldType)fld->flt;
			m_curr_state->field_part = FIELD_PART_PARAMS;
			switch (fld->flt)
			{
				case FLT_EMBED:
					doctotext_log(debug) << "Embedded OLE object reference found.";
					if (m_curr_state->obj_texts_iter == m_curr_state->obj_texts.end())
						doctotext_log(warning) << "Unexpected OLE object reference.";
					else
					{
						(*Text) += utf8_to_ustring(*m_curr_state->obj_texts_iter);
						m_curr_state->obj_texts_iter++;
					}
					break;
				default:
					m_curr_state->field_params = "";
					m_curr_state->field_value = "";
			}
		}

		void fieldSeparator(const FLD* fld,
			SharedPtr<const Word97::CHP> chp)
		{
			m_curr_state->field_part = FIELD_PART_VALUE;
		}

		void fieldEnd(const FLD* fld,
			SharedPtr<const Word97::CHP> chp)
		{
			UString params = m_curr_state->field_params;
			int i = 0;
			while (i < params.length() && params[i] == ' ') i++;
			UString field_type;
			while (i < params.length() && params[i] != ' ')
			{
				field_type += UString(params[i]);
				i++;
			}
			while (i < params.length() && params[i] == ' ') i++;
			params = params.substr(i);
			UString res_text;
			switch (m_curr_state->field_type)
			{
				case FLT_FILLIN:
					res_text = params + UString(" ") + m_curr_state->field_value;
					break;
				case FLT_EMBED:
					break;
				case FLT_HYPERLINK:
					if (params[0] == '"')
					{
						UString hyperlink_url;
						for (i = 1; i < params.length() && params[i] != '"'; i++)
							hyperlink_url += UString(params[i]);
						res_text = UString(formatUrl(ustring_to_string(hyperlink_url), ustring_to_string(m_curr_state->field_value), m_formatting).c_str());
					}
					else
						res_text = params + UString(" ") + m_curr_state->field_value;
					break;
				default:
					res_text = m_curr_state->field_value;
			}
			m_curr_state->field_type = FLT_NONE;
			m_curr_state->field_part = FIELD_PART_NONE;
			if (m_curr_state->table.in_table)
				m_curr_state->table.curr_cell_text += ustring_to_string(res_text);
			else if (m_curr_state->header_footer.in_header)
				m_curr_state->header_footer.header += res_text;
			else if (m_curr_state->header_footer.in_footer)
				m_curr_state->header_footer.footer += res_text;
			else
				(*Text) += res_text;
		}

		void endOfDocument()
		{
			if (m_comments_parsed)
				for (int i = 0; i < m_comments.size(); i++)
					if (m_comments[i].fc >= m_prev_par_fc)
						(*Text) += utf8_to_ustring(format_comment(m_comments[i].author, m_comments[i].text));
		}
};

class DocToTextTableHandler : public TableHandler
{
	private:
		UString* Text;
		CurrentTable* m_curr_table;

	public:
		DocToTextTableHandler(UString* text, CurrentTable* curr_table)
		{
			Text = text;
			m_curr_table = curr_table;
		}

		void tableRowStart(SharedPtr<const Word97::TAP> tap)
		{
			m_curr_table->in_table = true;
		}

		void tableRowEnd()
		{
			m_curr_table->rows.push_back(m_curr_table->curr_row_cells);
			m_curr_table->curr_row_cells.clear();
		}

		void tableCellStart()
		{
		}

		void tableCellEnd()
		{
			m_curr_table->curr_row_cells.push_back(m_curr_table->curr_cell_text);
			m_curr_table->curr_cell_text = "";
		}
};

class DocToTextSubDocumentHandler : public SubDocumentHandler
{
	private:
		CurrentHeaderFooter* m_curr_header_footer;
	
	public:
		DocToTextSubDocumentHandler(CurrentHeaderFooter* curr_header_footer)
			: m_curr_header_footer(curr_header_footer)
		{
		}

		virtual void headerStart(HeaderData::Type type)
		{
			switch (type)
			{
				case HeaderData::HeaderOdd:
				case HeaderData::HeaderEven:
				case HeaderData::HeaderFirst:
					m_curr_header_footer->in_header = true;
					break;
				case HeaderData::FooterOdd:
				case HeaderData::FooterEven:
				case HeaderData::FooterFirst:
					m_curr_header_footer->in_footer = true;
					break;
			}
		}

		virtual void headerEnd()
		{
			m_curr_header_footer->in_header = false;
			m_curr_header_footer->in_footer = false;
		}
};

struct DOCParser::Implementation
{
	const char* m_buffer;
	size_t m_buffer_size;
	std::string m_file_name;
  boost::signals2::signal<void(doctotext::Info &info)> m_on_new_node_signal;
};

DOCParser::DOCParser(const std::string& file_name, const std::shared_ptr<doctotext::ParserManager> &inParserManager)
: Parser(inParserManager)
{
	impl = NULL;
	try
	{
		impl = new Implementation();
		impl->m_file_name = file_name;
		impl->m_buffer = NULL;
		impl->m_buffer_size = 0;
	}
	catch (std::bad_alloc& ba)
	{
		if (impl)
			delete impl;
		throw;
	}
}

DOCParser::DOCParser(const char *buffer, size_t size, const std::shared_ptr<doctotext::ParserManager> &inParserManager)
: Parser(inParserManager)
{
	impl = NULL;
	try
	{
		impl = new Implementation();
		impl->m_file_name = "Memory buffer";
		impl->m_buffer = buffer;
		impl->m_buffer_size = size;
	}
	catch (std::bad_alloc& ba)
	{
		if (impl)
			delete impl;
		throw;
	}
}

DOCParser::~DOCParser()
{
	delete impl;
}

bool DOCParser::isDOC()
{
	FILE* f = NULL;
	if (!impl->m_buffer)
		f = fopen(impl->m_file_name.c_str(), "r");
	if (f == NULL && !impl->m_buffer)
		throw Exception("Error opening file " + impl->m_file_name);
	if (f)
		fclose(f);
	cerr_log_redirection cerr_redirection(doctotext_current_source_location());
	ThreadSafeOLEStorage* storage = NULL;
	//storage will be deleted inside parser from wv2 library
	if (impl->m_buffer)
		storage = new ThreadSafeOLEStorage(impl->m_buffer, impl->m_buffer_size);
	else
		storage = new ThreadSafeOLEStorage(impl->m_file_name);
	pthread_mutex_lock(&parser_factory_mutex_1);
	SharedPtr<wvWare::Parser> parser = ParserFactory::createParser(storage);
	pthread_mutex_unlock(&parser_factory_mutex_1);
	cerr_redirection.restore();
	if (!parser || !parser->isOk())
	{
		doctotext_log(error) << "Creating parser failed.";
		return false;
	}
	return true;
}

void DOCParser::getLinks(std::vector<doctotext::Link>& links)
{
	// warning TODO: Implement this functionality.
}

std::string DOCParser::plainText(const doctotext::FormattingStyle& formatting) const
{
	CurrentState curr_state;
	doctotext_log(debug) << "Opening " << impl->m_file_name << " as OLE file to parse all embedded objects in supported formats.";
	//Pointer to storage will be passed to parser from wv2 library. This pointer will be deleted, so allocate storage on heap
	ThreadSafeOLEStorage* storage = NULL;
	try
	{
		if (impl->m_buffer)
			storage = new ThreadSafeOLEStorage(impl->m_buffer, impl->m_buffer_size);
		else
			storage = new ThreadSafeOLEStorage(impl->m_file_name);
		if (!storage->isValid())
			throw Exception("Error opening " + impl->m_file_name + " as OLE file");
		if (storage->enterDirectory("ObjectPool"))
		{
			doctotext_log(debug) << "ObjectPool found, embedded OLE objects probably exist.";
			std::vector<std::string> obj_list;
			if (!storage->getStreamsAndStoragesList(obj_list))
				throw Exception("Error while loading list of streams and storages in ObjectPool directory. OLE Storage has reported an error: " + storage->getLastError());
			for (size_t i = 0; i < obj_list.size(); ++i)
			{
				doctotext_log(debug) << "OLE object entry found: " << obj_list[i];
				std::string obj_text;
				std::string currect_dir = obj_list[i];
				if (storage->enterDirectory(obj_list[i]))
				{
					std::vector<std::string> obj_list;
					if (!storage->getStreamsAndStoragesList(obj_list))
						throw Exception("Error while loading list of streams and storages in " + currect_dir + " directory. OLE Storage has reported an error: " + storage->getLastError());
					if (find(obj_list.begin(), obj_list.end(), "Workbook") != obj_list.end())
					{
						doctotext_log(debug) << "Embedded MS Excel workbook detected.";
						doctotext_log(debug) << "Using XLS parser.";
						try
						{
							XLSParser xls("");
							obj_text = xls.plainText(*storage, formatting);
						}
						catch (Exception& ex)
						{
							doctotext_log(error) << "Error while parsing embedded MS Excel workbook:\n" << ex.getBacktrace();
						}
					}
					storage->leaveDirectory();
				}
				curr_state.obj_texts.push_back(obj_text);
			}
		}
		else
		{
			doctotext_log(debug) << "No ObjectPool found, embedded OLE objects probably do not exist.";
		}
		storage->leaveDirectory();
		curr_state.obj_texts_iter = curr_state.obj_texts.begin();
	}
	catch (std::bad_alloc& ba)
	{
		if (storage)
			delete storage;
		throw;
	}
	catch (Exception& ex)
	{
		if (storage)
			delete storage;
		throw;
	}
	cerr_log_redirection cerr_redirection(doctotext_current_source_location());
	pthread_mutex_lock(&parser_factory_mutex_2);
	SharedPtr<wvWare::Parser> parser = ParserFactory::createParser(storage);
	pthread_mutex_unlock(&parser_factory_mutex_2);
	cerr_redirection.restore();
	if (!parser || !parser->isOk())
	{
		if (parser && parser->fib().fEncrypted)
			throw EncryptedFileException("File is encrypted");
		throw Exception("Creating parser failed");
	}
	UString text;
	DocToTextTextHandler text_handler(parser, &text, &curr_state, formatting);
	parser->setTextHandler(&text_handler);
	DocToTextTableHandler table_handler(&text, &curr_state.table);
	parser->setTableHandler(&table_handler);
	DocToTextSubDocumentHandler sub_document_handler(&curr_state.header_footer);
	parser->setSubDocumentHandler(&sub_document_handler);
	cerr_redirection.redirect();
	bool res = parser->parse();
	cerr_redirection.restore();
	if (!res)
		throw Exception("Parsing document failed");
	text_handler.endOfDocument();
	if (curr_state.header_footer.header != "")
		text = curr_state.header_footer.header + UString("\n") + text;
	if (curr_state.header_footer.footer != "")
		text += UString("\n") + curr_state.header_footer.footer;
	std::string s = ustring_to_string(text);
	// 0x0b character (vertical tab) is used as no-breaking carraige return.
	std::replace(s.begin(), s.end(), '\x0b', '\n');
  doctotext::Info new_info(StandardTag::TAG_TEXT, s);
  impl->m_on_new_node_signal(new_info);
	return s;
}

doctotext::Metadata DOCParser::metaData() const
{
  doctotext::Metadata meta;
	ThreadSafeOLEStorage* storage = NULL;
	try
	{
		if (impl->m_buffer)
			storage = new ThreadSafeOLEStorage(impl->m_buffer, impl->m_buffer_size);
		else
			storage = new ThreadSafeOLEStorage(impl->m_file_name);
		parse_oshared_summary_info(*storage, meta);
		delete storage;
		storage = NULL;
		return meta;
	}
	catch (std::bad_alloc& ba)
	{
		if (storage)
			delete storage;
		throw;
	}
	catch (Exception& ex)
	{
		if (storage)
			delete storage;
		throw;
	}
}

doctotext::Parser&
DOCParser::withParameters(const doctotext::ParserParameters &parameters)
{
	doctotext::Parser::withParameters(parameters);
	return *this;
}

void
DOCParser::parse() const
{
	doctotext_log(debug) << "Using DOC parser.";
  doctotext::FormattingStyle formating;
  plainText(formating);

  Metadata metadata = metaData();
  doctotext::Info metadata_info(StandardTag::TAG_METADATA, "", metadata.getFieldsAsAny());
  impl->m_on_new_node_signal(metadata_info);
}

doctotext::Parser&
DOCParser::addOnNewNodeCallback(doctotext::NewNodeCallback callback)
{
  impl->m_on_new_node_signal.connect(callback);
  return *this;
}
