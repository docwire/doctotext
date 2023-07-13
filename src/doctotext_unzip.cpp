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

#include "doctotext_unzip.h"

#include <iostream>
#include "log.h"
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "unzip.h"
#include "zlib.h"

namespace doctotext
{

const int CASESENSITIVITY = 1;

//data for reading from buffer (insted of file)
struct ZippedBuffer
{
	size_t m_size;
	const char* m_buffer;
	size_t m_pointer;
};

//following static functions will be used for reading from buffer:

static voidpf buffer_open(voidpf opaque, const char* filename, int mode)
{
	return (voidpf)1;	//if NULL, unzip library will consider this function has failed
}

static uLong buffer_read(voidpf opaque, voidpf stream, void* buf, uLong size)
{
	ZippedBuffer* buffer = (ZippedBuffer*)opaque;
	if (buffer->m_size < buffer->m_pointer + size)
	{
		size_t readed = buffer->m_size - buffer->m_pointer;
		memcpy(buf, buffer->m_buffer + buffer->m_pointer, readed);
		buffer->m_pointer = buffer->m_size;
		return readed;
	}
	else
	{
		memcpy(buf, buffer->m_buffer + buffer->m_pointer, size);
		buffer->m_pointer += size;
		return size;
	}
	return 0;
}

static uLong buffer_write(voidpf opaque, voidpf stream, const void* buf, uLong size)
{
	return 0;	//unimplemented, we dont need this
}

static long buffer_tell(voidpf opaque, voidpf stream)
{
	return ((ZippedBuffer*)opaque)->m_pointer;
}

static long buffer_seek(voidpf opaque, voidpf stream, uLong offset, int origin)
{
	ZippedBuffer* buffer = (ZippedBuffer*)opaque;
	size_t position;
	switch (origin)
	{
		case ZLIB_FILEFUNC_SEEK_CUR :
			position = buffer->m_pointer + offset;
			break;
		case ZLIB_FILEFUNC_SEEK_END :
			position = buffer->m_size - offset;
			break;
		case ZLIB_FILEFUNC_SEEK_SET :
			position = offset;
			break;
		default:
			return -1;
	}
	if (position > buffer->m_size)
		position = buffer->m_size;
	buffer->m_pointer = position;
	return 0;
}

static int buffer_close(voidpf opaque, voidpf stream)
{
	return 0;
}

static int buffer_error(voidpf opaque, voidpf stream)
{
	return 0;	//no errors at all?
}

struct DocToTextUnzip::Implementation
{
	std::string ArchiveFileName;
	unzFile ArchiveFile;
	std::map<std::string, unz_file_pos> m_directory;
	bool m_opened_for_chunks;
	bool m_from_memory_buffer;
	const char* m_buffer;
	size_t m_buffer_size;
	ZippedBuffer* m_zipped_buffer;
};

DocToTextUnzip::DocToTextUnzip()
{
	Impl = NULL;
	try
	{
		Impl = new Implementation();
		Impl->m_opened_for_chunks = false;
		Impl->m_from_memory_buffer = false;
		Impl->m_buffer = NULL;
		Impl->m_buffer_size = 0;
		Impl->ArchiveFile = NULL;
		Impl->m_zipped_buffer = NULL;
	}
	catch (std::bad_alloc& ba)
	{
		if (Impl)
			delete Impl;
		throw;
	}
}

DocToTextUnzip::DocToTextUnzip(const std::string& archive_file_name)
{
	Impl = NULL;
	try
	{
		Impl = new Implementation();
		Impl->ArchiveFileName = archive_file_name;
		Impl->m_opened_for_chunks = false;
		Impl->m_from_memory_buffer = false;
		Impl->m_buffer = NULL;
		Impl->m_buffer_size = 0;
		Impl->ArchiveFile = NULL;
		Impl->m_zipped_buffer = NULL;
	}
	catch (std::bad_alloc& ba)
	{
		if (Impl)
			delete Impl;
		throw;
	}
}

DocToTextUnzip::DocToTextUnzip(const char *buffer, size_t size)
{
	Impl = NULL;
	try
	{
		Impl = new Implementation();
		Impl->ArchiveFileName = "Memory buffer";
		Impl->m_opened_for_chunks = false;
		Impl->m_from_memory_buffer = true;
		Impl->m_buffer = buffer;
		Impl->m_buffer_size = size;
		Impl->ArchiveFile = NULL;
		Impl->m_zipped_buffer = NULL;
	}
	catch (std::bad_alloc& ba)
	{
		if (Impl)
			delete Impl;
		throw;
	}
}

void DocToTextUnzip::setArchiveFile(const std::string &archive_file_name)
{
	Impl->ArchiveFileName = archive_file_name;
}

void DocToTextUnzip::setBuffer(const char *buffer, size_t size)
{
	Impl->m_buffer = buffer;
	Impl->m_from_memory_buffer = true;
	Impl->m_buffer_size = size;
	Impl->ArchiveFileName = "Memory buffer";
}

DocToTextUnzip::~DocToTextUnzip()
{
	if (Impl->ArchiveFile != NULL)
		unzClose(Impl->ArchiveFile);
	if (Impl->m_zipped_buffer != NULL)
		delete Impl->m_zipped_buffer;
	delete Impl;
}

static std::string unzip_command;

void DocToTextUnzip::setUnzipCommand(const std::string& command)
{
	unzip_command = command;
}

bool DocToTextUnzip::open()
{
	if (!Impl->m_from_memory_buffer)
		Impl->ArchiveFile = unzOpen(Impl->ArchiveFileName.c_str());
	else
	{
		Impl->m_zipped_buffer = new ZippedBuffer;
		Impl->m_zipped_buffer->m_buffer = Impl->m_buffer;
		Impl->m_zipped_buffer->m_size = Impl->m_buffer_size;
		Impl->m_zipped_buffer->m_pointer = 0;
		zlib_filefunc_def read_from_buffer_functions;
		read_from_buffer_functions.zopen_file = &buffer_open;
		read_from_buffer_functions.zread_file = &buffer_read;
		read_from_buffer_functions.zwrite_file = &buffer_write;
		read_from_buffer_functions.ztell_file = &buffer_tell;
		read_from_buffer_functions.zseek_file = &buffer_seek;
		read_from_buffer_functions.zclose_file = &buffer_close;
		read_from_buffer_functions.zerror_file = &buffer_error;
		read_from_buffer_functions.opaque = Impl->m_zipped_buffer;
		//this function allows us to override default behaviour (reading from hard disc)
		Impl->ArchiveFile = unzOpen2(Impl->ArchiveFileName.c_str(), &read_from_buffer_functions);
	}
	if (Impl->ArchiveFile == NULL)
		return false;
	return true;
}

void DocToTextUnzip::close()
{
	unzClose(Impl->ArchiveFile);
	if (Impl->m_zipped_buffer)
		delete Impl->m_zipped_buffer;
	Impl->m_zipped_buffer = NULL;
	Impl->ArchiveFile = NULL;
}

bool DocToTextUnzip::exists(const std::string& file_name) const
{
	return (unzLocateFile(Impl->ArchiveFile, file_name.c_str(), CASESENSITIVITY) == UNZ_OK);
}

bool DocToTextUnzip::read(const std::string& file_name, std::string* contents, int num_of_chars) const
{
	// warning TODO: Add support for unzip command if Impl->m_from_memory_buffer == true
	if (unzip_command != "" && Impl->m_from_memory_buffer == false)
	{
		std::string temp_dir = tempnam(NULL, NULL);
		std::string cmd = unzip_command;
		size_t d_pos = cmd.find("%d");
		if (d_pos == std::string::npos)
		{
			doctotext_log(error) << "Unzip command must contain %d symbol.";
			return false;
		}
		cmd.replace(d_pos, 2, temp_dir);
		size_t a_pos = cmd.find("%a");
		if (a_pos == std::string::npos)
		{
			doctotext_log(error) << "Unzip command must contain %a symbol.";
			return false;
		}
		cmd.replace(a_pos, 2, Impl->ArchiveFileName);
		size_t f_pos = cmd.find("%f");
		if (f_pos == std::string::npos)
		{
			doctotext_log(error) << "Unzip command must contain %f symbol.";
			return false;
		}
		#ifdef WIN32
			std::string fn = file_name;
			size_t b_pos;
			while ((b_pos = fn.find('/')) != std::string::npos)
				fn.replace(b_pos, 1, "\\");
			cmd.replace(f_pos, 2, fn);
		#else
			cmd.replace(f_pos, 2, file_name);
		#endif
		cmd = cmd + " >&2";
		#ifdef WIN32
			const std::string remove_cmd = "rmdir /S /Q " + temp_dir;
		#else
			const std::string remove_cmd = "rm -rf " + temp_dir;
		#endif
		doctotext_log(debug) << "Executing " << cmd;
		if (system(cmd.c_str()) < 0)
			return false;
		FILE* f = fopen((temp_dir + "/" + file_name).c_str(), "r");
		if (f == NULL)
		{
			doctotext_log(debug) << "Executing " << remove_cmd;
			system(remove_cmd.c_str());
			return false;
		}
		try
		{
			*contents = "";
			char buffer[1024 + 1];
			int res;
			while((res = fread(buffer, sizeof(char), (num_of_chars > 0 && num_of_chars < 1024) ? num_of_chars : 1024, f)) > 0)
			{
				buffer[res] = '\0';
				*contents += buffer;
				if (num_of_chars > 0)
					if (contents->length() >= num_of_chars)
					{
						*contents = contents->substr(0, num_of_chars);
						break;
					}
			}
			if (res < 0)
			{
				fclose(f);
				f = NULL;
				doctotext_log(debug) << "Executing " << remove_cmd;
				system(remove_cmd.c_str());
				return false;
			}
			fclose(f);
			f = NULL;
			doctotext_log(debug) << "Executing " << remove_cmd;
			if (system(remove_cmd.c_str()) != 0)
				return false;
		}
		catch (std::bad_alloc& ba)
		{
			if (f)
				fclose(f);
			f = NULL;
			system(remove_cmd.c_str());
			throw;
		}
		return true;
	}

	int res;
	if (Impl->m_directory.size() > 0)
	{
		std::map<std::string, unz_file_pos>::iterator i = Impl->m_directory.find(file_name);
		if (i == Impl->m_directory.end())
			return false;
		res = unzGoToFilePos(Impl->ArchiveFile, &i->second);
	}
	else
		res = unzLocateFile(Impl->ArchiveFile, file_name.c_str(), CASESENSITIVITY);
	if (res != UNZ_OK)
		return false;
	res = unzOpenCurrentFile(Impl->ArchiveFile);
	if (res != UNZ_OK)
		return false;
	*contents = "";
	char buffer[1024 + 1];
	while((res = unzReadCurrentFile(Impl->ArchiveFile, buffer, (num_of_chars > 0 && num_of_chars < 1024) ? num_of_chars : 1024)) > 0)
	{
		buffer[res] = '\0';
		*contents += buffer;
		if (num_of_chars > 0)
			if (contents->length() >= num_of_chars)
			{
				*contents = contents->substr(0, num_of_chars);
				break;
			}
	}
	if (res < 0)
	{
		unzCloseCurrentFile(Impl->ArchiveFile);
		return false;
	}
	unzCloseCurrentFile(Impl->ArchiveFile);
	return true;
}

void DocToTextUnzip::closeReadingFileForChunks() const
{
	Impl->m_opened_for_chunks = false;
}

bool DocToTextUnzip::readChunk(const std::string& file_name, char* contents, int num_of_chars, int& readed) const
{
	if (Impl->m_opened_for_chunks == false)
	{
		int res;
		if (Impl->m_directory.size() > 0)
		{
			std::map<std::string, unz_file_pos>::iterator i = Impl->m_directory.find(file_name);
			if (i == Impl->m_directory.end())
				return false;
			res = unzGoToFilePos(Impl->ArchiveFile, &i->second);
		}
		else
			res = unzLocateFile(Impl->ArchiveFile, file_name.c_str(), CASESENSITIVITY);
		if (res != UNZ_OK)
			return false;
		res = unzOpenCurrentFile(Impl->ArchiveFile);
		if (res != UNZ_OK)
			return false;
		Impl->m_opened_for_chunks = true;
	}
	readed = unzReadCurrentFile(Impl->ArchiveFile, contents, num_of_chars);
	if (readed < 0)
	{
		unzCloseCurrentFile(Impl->ArchiveFile);
		return false;
	}
	if(readed < num_of_chars)	//end of file detected
	{
		contents[readed] = '\0';
		unzCloseCurrentFile(Impl->ArchiveFile);
		Impl->m_opened_for_chunks = false;
		return true;
	}
	contents[readed] = '\0';
	return true;
}

bool DocToTextUnzip::readChunk(const std::string& file_name, std::string* contents, int num_of_chars) const
{
	std::vector<char> vcontents(num_of_chars + 1);
	int readed;
	if (!readChunk(file_name, &vcontents[0], num_of_chars, readed))
		return false;
	if (readed == 0)
		*contents = "";
	else
		contents->assign(vcontents.begin(), vcontents.begin() + readed);
	return true;
}

bool DocToTextUnzip::getFileSize(const std::string& file_name, unsigned long& file_size) const
{
	int res;
	unz_file_info file_info;
	if (Impl->m_directory.size() > 0)
	{
		std::map<std::string, unz_file_pos>::iterator i = Impl->m_directory.find(file_name);
		if (i == Impl->m_directory.end())
			return false;
		res = unzGoToFilePos(Impl->ArchiveFile, &i->second);
	}
	else
		res = unzLocateFile(Impl->ArchiveFile, file_name.c_str(), CASESENSITIVITY);
	if (res != UNZ_OK)
		return false;
	if (unzGetCurrentFileInfo(Impl->ArchiveFile, &file_info, NULL, 0, NULL, 0, NULL, 0) != UNZ_OK)
		return false;
	file_size = file_info.uncompressed_size;
	return true;
}

bool DocToTextUnzip::loadDirectory()
{
	Impl->m_directory.clear();
	if (unzGoToFirstFile(Impl->ArchiveFile) != UNZ_OK)
		return false;
	for (;;)
	{
		char name[1024];
		if (unzGetCurrentFileInfo(Impl->ArchiveFile, NULL, name, 1024, NULL, 0, NULL, 0) != UNZ_OK)
			return false;
		unz_file_pos pos;
		if (unzGetFilePos(Impl->ArchiveFile, &pos) != UNZ_OK)
			return false;
		Impl->m_directory[name] = pos;
		int res = unzGoToNextFile(Impl->ArchiveFile);
		if (res == UNZ_END_OF_LIST_OF_FILE)
			break;
		if (res != UNZ_OK)
			return false;
	}
	return true;
}

}; // namespace doctotext
