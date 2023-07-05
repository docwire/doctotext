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

#include "decompress_archives.h"

#include <archive.h>
#include <archive_entry.h>
#include "exception.h"
#include <filesystem>
#include <fstream>
#include "parser.h"

namespace doctotext
{

DecompressArchives::DecompressArchives()
{
}

DecompressArchives::DecompressArchives(const DecompressArchives &other)
{
}

DecompressArchives::~DecompressArchives()
{
}

class ArchiveReader
{
public:
	class EntryStreamBuf : public std::streambuf
	{
	public:
		EntryStreamBuf(archive* archive)
			: m_archive(archive)
		{
		}

		int_type underflow()
		{
			//std::cerr << "underflow()" << std::endl;
			la_ssize_t bytes_read = archive_read_data(m_archive, m_buffer, m_buf_size);
			//std::cerr << "bytes read: " << bytes_read << std::endl;
			if (bytes_read < 0)
				throw Exception(archive_error_string(m_archive));
			if (bytes_read == 0)
				return traits_type::eof();
			setg(m_buffer, m_buffer, m_buffer + bytes_read);
			return traits_type::to_int_type(*gptr());
		}

	private:
		archive* m_archive;
		static constexpr size_t m_buf_size = 16384;
		char m_buffer[m_buf_size];
	};

	class EntryIStream : public std::istream
	{
	public:
		EntryIStream(archive* archive)
			: std::istream(new EntryStreamBuf(archive)) {}

		~EntryIStream() { delete rdbuf(); }
	};

	class Entry
	{
	public:
		Entry(archive* archive, archive_entry* entry)
			: m_archive(archive), m_entry(entry) {}

		std::string get_name() { return archive_entry_pathname(m_entry); }

		bool is_dir() { return (archive_entry_mode(m_entry) & AE_IFDIR); }

		EntryIStream create_stream() { return EntryIStream(m_archive); }

		operator bool() { return m_entry != nullptr; }

		bool operator!= (const Entry& r) { return m_entry != r.m_entry; };

	private:
		archive* m_archive;
		archive_entry* m_entry;
	};

	struct Iterator
	{
		ArchiveReader* m_reader;
		Entry m_entry;
		Iterator(ArchiveReader* reader, Entry entry)
			: m_reader(reader),
				m_entry(entry)
		{
		}
		Iterator& operator++() { m_entry = m_reader->get_next_entry(); return *this; }
		bool operator!= (const Iterator& r) { return m_entry != r.m_entry; };
		const Entry& operator*() const { return m_entry; }
	};

	ArchiveReader(std::istream& stream)
		: data(stream)
	{
		m_archive = archive_read_new();
		archive_read_support_filter_all(m_archive);
		archive_read_support_format_all(m_archive);
		int r = archive_read_open(m_archive, &data, nullptr, archive_read_callback, archive_close_callback);
		if (r != ARCHIVE_OK)
			throw Exception(archive_error_string(m_archive));
	}

	~ArchiveReader()
	{
		int r = archive_read_free(m_archive);
		//if (r != ARCHIVE_OK)
		//	std::cerr << "archive_read_free() error: " << archive_error_string(m_archive) << std::endl;
	}

	archive* get_archive()
	{
		return m_archive;
	}

	Entry get_next_entry()
	{
		archive_entry* entry;
		int r = archive_read_next_header(m_archive, &entry);
		if (r == ARCHIVE_EOF)
		{
			//std::cerr << "End of archive" << std::endl;
			return Entry(m_archive, nullptr);
		}
		if (r != ARCHIVE_OK)
		{
			//std::cerr << "archive_read_next_header() error: " << archive_error_string(m_archive) << std::endl;
			throw Exception(archive_error_string(m_archive));
		}
		return Entry(m_archive, entry);
	}

	Iterator begin()
	{
		return Iterator(this, get_next_entry());
	}

	Iterator end()
	{
		return Iterator(this, Entry(m_archive, nullptr));
	}

private:
	struct CallbackClientData
	{
		CallbackClientData(std::istream& stream)
			: m_stream(stream)
		{
		}
		std::istream& m_stream;
		static constexpr size_t m_buf_size = 16384;
		char m_buffer[m_buf_size];
	};

	archive* m_archive;
	CallbackClientData data;

	static la_ssize_t archive_read_callback(archive* archive, void* client_data, const void** buf)
	{
		//std::cerr << "archive_read_callback()" << std::endl;
		CallbackClientData* data = (CallbackClientData*)client_data;
		*buf = data->m_buffer;
		if (data->m_stream.read(data->m_buffer, data->m_buf_size))
			return data->m_buf_size;
		else
		{
			if (!data->m_stream.eof())
			{
				archive_set_error(archive, EIO, "Stream reading error");
				return -1;
			}
			return data->m_stream.gcount();
		}
	}

	static int archive_close_callback(archive* archive, void* client_data)
	{
		return ARCHIVE_OK;
	}
};

void
DecompressArchives::process(doctotext::Info &info) const
{
	//std::cerr << "Decompress archives" << std::endl;
	if (info.tag_name != StandardTag::TAG_FILE)
	{
		emit(info);
		return;
	}
	//std::cerr << "TAG_FILE" << std::endl;
	std::optional<std::string> path = info.getAttributeValue<std::string>("path");
	std::optional<std::istream*> stream = info.getAttributeValue<std::istream*>("stream");
	std::optional<std::string> name = info.getAttributeValue<std::string>("name");
	if(!path && !stream)
		throw Exception("No path or stream in TAG_FILE");
	auto is_supported = [](const std::string& fn)
	{
		std::set<std::string> supported_extensions { ".zip", ".tar", ".rar", ".gz", ".bz2", ".xz" };
		return supported_extensions.count(std::filesystem::path(fn).extension().string()) > 0;
	};
	if ((path && !is_supported(*path)) || (name && !is_supported(*name)))
	{
		//std::cerr << "Filename extension shows it is not an supported archive, skipping." << std::endl;
		emit(info);
		return;
	}
	std::istream* in_stream = path ? new std::ifstream ((*path).c_str(), std::ios::binary ) : *stream;
	try
	{
		//std::cerr << "Decompressing archive" << std::endl;
		ArchiveReader reader(*in_stream);
		for (ArchiveReader::Entry entry: reader)
		{
			std::string entry_name = entry.get_name();
			//std::cerr << "Processing compressed file " << entry_name << std::endl;
			if (entry.is_dir())
			{
				//std::cerr << "Skipping directory entry" << std::endl;
				continue;
			}
			ArchiveReader::EntryIStream entry_stream = entry.create_stream();
			Info info(StandardTag::TAG_FILE, "", {{"stream", (std::istream*)&entry_stream}, {"name", entry_name}});
			process(info);
			//std::cerr << "End of processing compressed file " << entry_name << std::endl;
		}
		//std::cerr << "Archive decompressed successfully" << std::endl;
	}
	catch (Exception& e)
	{
		//std::cerr << e.what() << std::endl;
		in_stream->clear();
		in_stream->seekg(std::ios::beg);
		emit(info);
	}
	if (path)
		delete in_stream;
}

DecompressArchives* DecompressArchives::clone() const
{
	return new DecompressArchives(*this);
}

} // namespace doctotext
