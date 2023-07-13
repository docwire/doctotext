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

#include "oshared.h"

#include "exception.h"
#include "misc.h"
#include <iostream>
#include "log.h"
#include "metadata.h"
#include "wv2/utilities.h"
#include <time.h>
#include "thread_safe_ole_stream_reader.h"
#include "thread_safe_ole_storage.h"

using namespace wvWare;

static bool read_vt_string(ThreadSafeOLEStreamReader* reader, std::string& s)
{
	U16 string_type;
	if (!reader->readU16(string_type) || string_type != 0x1E)
	{
		doctotext_log(error) << "Incorrect string type.";
		return false;
	}
	reader->seek(2, SEEK_CUR); //padding
	U32 len;
	reader->readU32(len);
	s = "";
	for (int j = 0; j < len - 1; j++)
	{
		S8 ch;
		reader->readS8(ch);
		if (ch == '\0')
			break;
		s += ch;
	}
	if (!reader->isValid())
	{
		doctotext_log(error) << "OLE Reader error message: " << reader->getLastError();
		return false;
	}
	return true;
}

static bool read_vt_i4(ThreadSafeOLEStreamReader* reader, S32& i)
{
	U16 string_type;
	if (!reader->readU16(string_type) || string_type != 0x0003)
	{
		doctotext_log(error) << "Incorrect value type.";
		return false;
	}
	reader->seek(2, SEEK_CUR); //padding
	reader->readS32(i);
	if (!reader->isValid())
	{
		doctotext_log(error) << reader->getLastError();
		return false;
	}
	return true;
}

static bool read_vt_i2(ThreadSafeOLEStreamReader* reader, S16& i)
{
	U16 string_type;
	if (!reader->readU16(string_type) || string_type != 0x0002)
	{
		doctotext_log(error) << "Incorrect value type.";
		return false;
	}
	reader->seek(2, SEEK_CUR); //padding
	reader->readS16(i);
	if (!reader->isValid())
	{
		doctotext_log(error) << "OLE Reader error message: " << reader->getLastError();
		return false;
	}
	return true;
}

static bool read_vt_filetime(ThreadSafeOLEStreamReader* reader, tm& time)
{
	U16 type;
	if (!reader->readU16(type) || type != 0x0040)
	{
		doctotext_log(error) << "Incorrect variable type.";
		return false;
	}
	reader->seek(2, SEEK_CUR); //padding
	U32 file_time_low, file_time_high;
	reader->readU32(file_time_low);
	reader->readU32(file_time_high);
	if (!reader->isValid())
	{
		doctotext_log(error) << "OLE Reader error message: " << reader->getLastError();
		return false;
	}
	if (file_time_low == 0 && file_time_high == 0)
	{
		// Sometimes field exists but date is zero.
		// Last modification time saved by LibreOffice 3.5 when document is created is an example.
		time = tm();
		return true;
	}
	unsigned long long int file_time = ((unsigned long long int)file_time_high << 32) | (unsigned long long int)file_time_low;
	time_t t = (time_t)(file_time / 10000000 - 11644473600LL);
  struct tm time_buffer;
  tm* res = thread_safe_gmtime(&t, time_buffer);
	if (res == NULL)
	{
		doctotext_log(error) << "Incorrect time value.";
		return false;
	}
	time = *res;
	return true;
}

void parse_oshared_summary_info(ThreadSafeOLEStorage& storage, Metadata& meta)
{
	doctotext_log(debug) << "Extracting metadata.";
	if (!storage.isValid())
		throw Exception("Error opening " + storage.name() + " as OLE file");
	ThreadSafeOLEStreamReader* reader = NULL;
	reader = (ThreadSafeOLEStreamReader*)storage.createStreamReader("\005SummaryInformation");
	if (reader == NULL)
		throw Exception("Error opening SummaryInformation stream");
	try
	{
		size_t field_set_stream_start = reader->tell();
		U16 byte_order;
		if (!reader->readU16(byte_order) || byte_order != 0xFFFE)
			throw Exception("Incorrect ByteOrder value");
		U16 version;
		if (!reader->readU16(version) || version != 0x00)
			throw Exception("Incorrect Version value");
		reader->seek(4, SEEK_CUR); //system indentifier
		for (int i = 0; i < 4; i++)
		{
			U32 clsid_part;
			if (!reader->readU32(clsid_part) || clsid_part != 0x00)
				throw Exception("Incorrect CLSID value");
		}
		U32 num_property_sets;
		if (!reader->readU32(num_property_sets) || (num_property_sets != 0x01 && num_property_sets != 0x02))
			throw Exception("Incorrect number of property sets");
		reader->seek(16, SEEK_CUR);	// fmtid0_part
		U32 offset;
		reader->readU32(offset);
		int property_set_pos = field_set_stream_start + offset;
		reader->seek(property_set_pos, SEEK_SET);
		U32 size, num_props = 0;
		reader->readU32(size);
		reader->readU32(num_props);
		for (int i = 0; i < num_props; i++)
		{
			U32 prop_id, offset;
			reader->readU32(prop_id);
			reader->readU32(offset);
			int p = reader->tell();
			switch (prop_id)
			{
				case 0x00000004:
				{
					reader->seek(property_set_pos + offset, SEEK_SET);
					std::string author;
					if (read_vt_string(reader, author))
					{
						meta.setAuthor(author);
						meta.setAuthorType(Metadata::EXTRACTED);
					}
					break;
				}
				case 0x00000008:
				{
					reader->seek(property_set_pos + offset, SEEK_SET);
					std::string last_modified_by;
					if (read_vt_string(reader, last_modified_by))
					{
						meta.setLastModifiedBy(last_modified_by);
						meta.setLastModifiedByType(Metadata::EXTRACTED);
					}
					break;
				}
				case 0x0000000C:
				{
					reader->seek(property_set_pos + offset, SEEK_SET);
					tm creation_date;
					if (read_vt_filetime(reader, creation_date))
					{
						meta.setCreationDate(creation_date);
						meta.setCreationDateType(Metadata::EXTRACTED);
					}
					break;
				}
				case 0x0000000D:
				{
					reader->seek(property_set_pos + offset, SEEK_SET);
					tm last_modification_date;
					if (read_vt_filetime(reader, last_modification_date))
					{
						meta.setLastModificationDate(last_modification_date);
						meta.setLastModificationDateType(Metadata::EXTRACTED);
					}
					break;
				}
				case 0x0000000E:
				{
					reader->seek(property_set_pos + offset, SEEK_SET);
					S32 page_count;
					if (read_vt_i4(reader, page_count))
					{
						meta.setPageCount(page_count);
						meta.setPageCountType(Metadata::EXTRACTED);
					}
					break;
				}
				case 0x0000000F:
				{
					reader->seek(property_set_pos + offset, SEEK_SET);
					S32 word_count;
					if (read_vt_i4(reader, word_count))
					{
						meta.setWordCount(word_count);
						meta.setWordCountType(Metadata::EXTRACTED);
					}
					break;
				}
			}
			reader->seek(p, SEEK_SET);
			if (!reader->isValid())
				throw Exception("OLE Reader error message: " + reader->getLastError());
		}
		if (!reader->isValid())
			throw Exception("OLE Reader error message: " + reader->getLastError());
		delete reader;
	}
	catch (std::bad_alloc& ba)
	{
		if (reader)
			delete reader;
		reader = NULL;
		throw;
	}
	catch (Exception& ex)
	{
		if (reader)
			delete reader;
		reader = NULL;
		ex.appendError("Error while parsing SummaryInformation stream");
		throw;
	}
}

static ThreadSafeOLEStreamReader* open_oshared_document_summary_info(ThreadSafeOLEStorage& storage, size_t& field_set_stream_start)
{
	ThreadSafeOLEStreamReader* reader = NULL;
	try
	{
		reader = (ThreadSafeOLEStreamReader*)storage.createStreamReader("\005DocumentSummaryInformation");
		if (reader == NULL)
			throw Exception("Error opening DocumentSummaryInformation stream");
		field_set_stream_start = reader->tell();
		U16 byte_order;
		if (!reader->readU16(byte_order) || byte_order != 0xFFFE)
			throw Exception("Incorrect ByteOrder value");
		U16 version;
		if (!reader->readU16(version) || version != 0x00)
			throw Exception("Incorrect Version value");
		reader->seek(4, SEEK_CUR);	// system indentifier
		for (int i = 0; i < 4; i++)
		{
			U32 clsid_part;;
			if (!reader->readU32(clsid_part) || clsid_part != 0x00)
				throw Exception("Incorrect CLSID value");
		}
		U32 num_property_sets;
		if (!reader->readU32(num_property_sets) || (num_property_sets != 0x01 && num_property_sets != 0x02))
			throw Exception("Incorrect number of property sets");
		reader->seek(16, SEEK_CUR);	//fmtid0_part
		if (!reader->isValid())
			throw Exception("OLE Reader error message: " + reader->getLastError());
		return reader;
	}
	catch (std::bad_alloc& ba)
	{
		if (reader)
			delete reader;
		reader = NULL;
		throw;
	}
	catch (Exception& ex)
	{
		if (reader)
			delete reader;
		reader = NULL;
		throw;
	}
}

bool get_codepage_from_document_summary_info(ThreadSafeOLEStorage& storage, std::string& codepage)
{
	size_t field_set_stream_start;
	if (!storage.isValid())
		doctotext_log(error) << "Error opening " << storage.name() << " as OLE file.";
	ThreadSafeOLEStreamReader* reader = NULL;
	try
	{
		reader = open_oshared_document_summary_info(storage, field_set_stream_start);
		U32 offset;
		reader->readU32(offset);
		int property_set_pos = field_set_stream_start + offset;
		reader->seek(property_set_pos, SEEK_SET);
		U32 size, num_props = 0;
		reader->readU32(size);
		reader->readU32(num_props);
		for (int i = 0; i < num_props; i++)
		{
			U32 prop_id, offset;
			reader->readU32(prop_id);
			reader->readU32(offset);
			int p = reader->tell();
			switch (prop_id)
			{
				case 0x00000001:
					reader->seek(property_set_pos + offset, SEEK_SET);
					S16 icodepage;
					if (!read_vt_i2(reader, icodepage))
					{
						doctotext_log(error) << "Error while reading codepage: invalid value.";
						delete reader;
						return false;
					}
					codepage = "cp" + int2string(icodepage);
					delete reader;
					return true;
			}
			reader->seek(p, SEEK_SET);
			if (!reader->isValid())
			{
				doctotext_log(error) << "OLE Reader error message: " << reader->getLastError();
				delete reader;
				return false;
			}
		}
		if (!reader->isValid())
		{
			doctotext_log(error) << "OLE Reader error message: " << reader->getLastError();
			delete reader;
			return false;
		}
	}
	catch (std::bad_alloc& ba)
	{
		if (reader)
			delete reader;
		reader = NULL;
		doctotext_log(error) << "Error while getting codepage info. Reason: bad_alloc.";
		throw;
	}
	catch (Exception& ex)
	{
		if (reader)
			delete reader;
		reader = NULL;
		doctotext_log(error) << "Error while getting codepage info. Error message:" << ex.getBacktrace();
		return false;
	}
	delete reader;
	doctotext_log(warning) << "Information about codepage is missing.";
	return false;
}

void parse_oshared_document_summary_info(ThreadSafeOLEStorage& storage, int& slide_count)
{
	doctotext_log(debug) << "Extracting additional metadata.";
	size_t field_set_stream_start;
	if (!storage.isValid())
		throw Exception("Error opening " + storage.name() + " as OLE file");
	ThreadSafeOLEStreamReader* reader = NULL;
	try
	{
		reader = open_oshared_document_summary_info(storage, field_set_stream_start);
		U32 offset;
		reader->readU32(offset);
		int property_set_pos = field_set_stream_start + offset;
		reader->seek(property_set_pos, SEEK_SET);
		U32 size, num_props = 0;
		reader->readU32(size);
		reader->readU32(num_props);
		bool slide_count_found = false;
		for (int i = 0; i < num_props; i++)
		{
			U32 prop_id, offset;
			reader->readU32(prop_id);
			reader->readU32(offset);
			int p = reader->tell();
			switch (prop_id)
			{
				case 0x00000007:
					reader->seek(property_set_pos + offset, SEEK_SET);
					if (read_vt_i4(reader, slide_count))
						slide_count_found = true;
					break;
			}
			reader->seek(p, SEEK_SET);
			if (!reader->isValid())
				throw Exception("OLE reader error message: " + reader->getLastError());
		}
		if (!slide_count_found)
			slide_count = -1;
		if (!reader->isValid())
			throw Exception("OLE reader error message: " + reader->getLastError());
		delete reader;
		reader = NULL;
	}
	catch (std::bad_alloc& ba)
	{
		if (reader)
			delete reader;
		reader = NULL;
		throw;
	}
	catch (Exception& ex)
	{
		if (reader)
			delete reader;
		reader = NULL;
		ex.appendError("Error while parsing DocumentSummaryInformation stream");
		throw;
	}
}
