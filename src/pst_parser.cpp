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


#include <fstream>
#include <iomanip>
#include <ctime>

#include <optional>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <memory>
#include <chrono>
#include <sstream>
extern "C"
{
#define LIBPFF_HAVE_BFIO
#include <stdio.h>
#include <libpff.h>
 #include <libbfio.h>
}

#include <boost/signals2.hpp>

#include "formatting_style.h"
#include "log.h"
#include "misc.h"
#include "pst_parser.h"

inline std::string toString(const uint8_t* text)
{
	if (text == nullptr)
		return "";
	return {(const char*)text};
}

static doctotext::FormattingStyle formatStyle(doctotext::TABLE_STYLE_TABLE_LOOK, doctotext::URL_STYLE_EXTENDED);

constexpr uint64_t WINDOWS_TICK = 10000000;
constexpr uint64_t SHIFT = 11644473600;

inline std::unique_ptr<char[]> getAttachmentName(libpff_item_t* item)
{
	size_t size;
	if (libpff_attachment_get_utf8_short_filename_size(item, &size, nullptr) != 1)
	{
		return nullptr;
	}
	std::unique_ptr<char[]> out(std::make_unique<char[]>(size + 1));
	auto* filename = (uint8_t*)out.get();
	if (libpff_attachment_get_utf8_short_filename(item, filename, size, nullptr) != 1)
	{
		return nullptr;
	}
	return out;
}

/**
	TODO: Consider changing these to just unique_ptr to avoid boilerplate
*/

struct pffError
{
	libpff_error_t* error = nullptr;
	pffError() = default;
	pffError(libpff_error_t* error) : error{error} {}
	~pffError()
	{
		libpff_error_free(&error);
	}
	pffError& operator=(pffError&& in)
	{
		auto ptr = std::exchange(error, std::exchange(in.error, nullptr));
		libpff_error_free(&ptr);
		return *this;
	}
	pffError(const pffError&) = delete;
	pffError& operator=(const pffError&) = delete;
	pffError(pffError&& in)
	{
		auto ptr = std::exchange(error, std::exchange(in.error, nullptr));
		libpff_error_free(&ptr);
	}
	operator libpff_error_t*() const
	{
		return error;
	}
	auto operator&() const
	{
		return &error;
	}
	operator libpff_error_t*()
	{
		return error;
	}
	auto operator&()
	{
		return &error;
	}
};

struct pffItem
{
	libpff_item_t* handle = nullptr;
	pffItem() = default;
	pffItem(libpff_item_t* in) : handle{in}
	{
	}
	~pffItem()
	{
//		libpff_item_free(&handle, nullptr);
	}
	pffItem& operator=(pffItem&& in)
	{
		auto ptr = std::exchange(handle, std::exchange(in.handle, nullptr));
		libpff_item_free(&ptr, nullptr);
		return *this;
	}
	pffItem(const pffItem&) = delete;
	pffItem& operator=(const pffItem& in) = delete;
	pffItem(pffItem&& in)
	{
		auto ptr = std::exchange(handle, std::exchange(in.handle, nullptr));
		libpff_item_free(&ptr, nullptr);
	}
	operator libpff_item_t*() const
	{
		return handle;
	}
	auto operator&() const
	{
		return &handle;
	}
	operator libpff_item_t*()
	{
		return handle;
	}
	auto operator&()
	{
		return &handle;
	}
};

struct RawAttachment
{
  RawAttachment(size_t size, std::unique_ptr<uint8_t[]>& raw_data, std::string name)
  : m_size(size),
    m_raw_data(std::move(raw_data)),
    m_name(name)
  {}
  std::string m_name;
  size_t m_size;
  std::unique_ptr<uint8_t[]> m_raw_data;
};

class Message
{
  public:
	explicit Message(pffItem in)
		: _messageHandle{std::move(in)}
	{
	}

	std::string getName() const
	{
		pffError error;
		size_t name_size;
    if (libpff_message_get_utf8_subject_size(_messageHandle, &name_size, &error) == 1)
    {
      std::unique_ptr<uint8_t[]> name = std::make_unique<uint8_t[]>(name_size);
      if (libpff_message_get_utf8_subject(_messageHandle, name.get(), name_size, &error) == 1)
      {
        return toString(name.get());
      }
      return "";
    }
    return "";
	}

  std::string getMailSender() const
  {
    pffError error;
    size_t name_size;
    if (libpff_message_get_utf8_sender_email_address_size(_messageHandle, &name_size, &error) == 1)
    {
      std::unique_ptr<uint8_t[]> name = std::make_unique<uint8_t[]>(name_size);
      if (libpff_message_get_utf8_sender_email_address(_messageHandle, name.get(), name_size, &error) == 1)
      {
        return toString(name.get());
      }
      return "";
    }
    return "";
  }

  std::string getMailRecipient() const
  {
    pffError error;
    size_t name_size;
    if (libpff_message_get_utf8_received_by_email_address_size(_messageHandle, &name_size, &error) == 1)
    {
      std::unique_ptr<uint8_t[]> name = std::make_unique<uint8_t[]>(name_size);
      if (libpff_message_get_utf8_received_by_email_address(_messageHandle, name.get(), name_size, &error) == 1)
      {
        return toString(name.get());
      }
      return "";
    }
    return "";
  }

	std::string getCreationDateAsString() const
	{
		std::time_t temp = getCreationDate();
    struct tm time_buffer;
		std::tm* t = thread_safe_gmtime(&temp, time_buffer);
		std::stringstream stream;
		stream << std::put_time(t, "%Y-%m-%d %I:%M:%S %p");
		return stream.str();
	}

  uint32_t getCreationDate() const
  {
    uint64_t creation_time;
    libpff_message_get_creation_time(_messageHandle, &creation_time, nullptr);

    creation_time = creation_time / WINDOWS_TICK - SHIFT;
    return static_cast<uint32_t>(creation_time);
  }

	std::optional<std::string> getTextAsRtf() const
	{
    size_t sizeOfBody;
    if (libpff_message_get_rtf_body_size(_messageHandle, &sizeOfBody, nullptr) == 1)
    {
      std::string text(sizeOfBody, 0);
      libpff_message_get_rtf_body(_messageHandle, (uint8_t *) text.data(), text.size(), nullptr);
      return text;
    }
    return {};
	}

	std::optional<std::string> getTextAsHtml() const
	{
    size_t sizeOfBody;
    if (libpff_message_get_html_body_size(_messageHandle, &sizeOfBody, nullptr) == 1)
    {
      std::string text(sizeOfBody, 0);
      libpff_message_get_html_body(_messageHandle, (uint8_t *) text.data(), text.size(), nullptr);
      return text;
    }
    return {};
	}

	std::vector<RawAttachment>
	getAttachments()
	{
		int items;
		pffError err;
    std::vector<RawAttachment> attachments;
    if (libpff_message_get_number_of_attachments(_messageHandle, &items, &err) != 1)
		{
			return {};
		}
		for (int i = 0; i < items; ++i)
		{
			pffItem item;
			if (libpff_message_get_attachment(_messageHandle, i, &item, &err) != 1)
			{
				doctotext_log(debug) << "Message Get attachments failed: ";
				continue;
			}
			size64_t size;
			if (libpff_attachment_get_data_size(item, &size, &err) == -1)
			{
				doctotext_log(debug) << "Get data size failed";
				continue;
			}
			std::unique_ptr<uint8_t[]> datablob(new uint8_t[size + 1]);
			if (libpff_attachment_data_read_buffer(item, datablob.get(), size, &err) == -1)
			{
				doctotext_log(debug) << "Data Read failed";
				continue;
			}
      std::string attachment_name = getAttachmentName(item).get();
      attachments.push_back(RawAttachment(size, datablob, attachment_name));
		}
		return attachments;
	}

  private:
	pffItem _messageHandle;
};

class Folder
{
  public:
	explicit Folder(pffItem folderHandle)
		: _folderHandle(std::move(folderHandle))
	{
	}

	int getSubFolderNumber() const
	{
		int folders_no;
		libpff_folder_get_number_of_sub_folders(_folderHandle, &folders_no, nullptr);
		return folders_no;
	}

	int getMessageNumber() const
	{
		int messages_no;
		libpff_folder_get_number_of_sub_messages(_folderHandle, &messages_no, nullptr);
		return messages_no;
	}

	Folder getSubFolder(int index) const
	{
		libpff_item_t* sub_folder = NULL;
		libpff_folder_get_sub_folder(_folderHandle, index, &sub_folder, nullptr);
		Folder folder(sub_folder);
		return folder;
	}

	Message getMessage(int index) const
	{
		libpff_item_t* mail = NULL;
		libpff_folder_get_sub_message(_folderHandle, index, &mail, nullptr);
		Message msg(mail);
		return msg;
	}

	std::string getName() const
	{
		size_t name_size;
		libpff_folder_get_utf8_name_size(_folderHandle, &name_size, nullptr);
		std::unique_ptr<uint8_t[]> name = std::make_unique<uint8_t[]>(name_size);
		libpff_folder_get_utf8_name(_folderHandle, name.get(), name_size, nullptr);
		return toString(name.get());
	}

  private:
	pffItem _folderHandle;
};

struct PSTParser::Implementation
{
	Implementation(std::string file_name, PSTParser* owner)
		: m_file_name(std::move(file_name)),
      m_owner(owner)
	{
		std::filesystem::path path = m_file_name;
		m_file_name = path.make_preferred().string();
	}

  Implementation(const char* buffer, size_t size, PSTParser* owner)
  : m_buffer(buffer),
    m_size(size),
    m_owner(owner)
  {
  }

	void parse() const;

  void onNewNode(doctotext::NewNodeCallback callback)
  {
    m_on_new_node_signal.connect(callback);
  }

  void addParameter(const doctotext::ParserParameters &parameters)
  {
    m_parameters = parameters;
  }

  PSTParser* m_owner;
  doctotext::ParserParameters m_parameters;
  bool m_error;
  std::string m_file_name;
  const char* m_buffer;
  size_t m_size;
  std::istream *m_data_stream;
  boost::signals2::signal<void(doctotext::Info &info)> m_on_new_node_signal;
  std::shared_ptr<doctotext::ParserManager> parserManager;
  const unsigned int MAILS_LIMIT = 50;

  private:
    void parse_element(const char* buffer, size_t size, const std::string& extension="") const;
    void parse_internal(const Folder& root, int deep, unsigned int &mail_counter) const;
};

void
PSTParser::Implementation::parse_element(const char* buffer, size_t size, const std::string& extension) const
{
  if (parserManager)
  {
    auto parser_builder = parserManager->findParserByExtension(extension);
    if (parser_builder)
    {
      (*parser_builder)->withParserManager(parserManager)
        .withOnNewNodeCallbacks({[this](doctotext::Info &info){m_owner->sendTag(info.tag_name, info.plain_text, info.attributes);}})
        .withParameters(m_parameters)
        .build(buffer, size)
        ->parse();
    }
  }
}

void PSTParser::Implementation::parse_internal(const Folder& root, int deep, unsigned int &mail_counter) const
{
	for (int i = 0; i < root.getSubFolderNumber(); ++i)
	{
		auto sub_folder = root.getSubFolder(i);
    auto callback = m_owner->sendTag(StandardTag::TAG_FOLDER, "", {{"name", sub_folder.getName()}, {"level", deep}});
    if(callback.skip)
    {
      continue;
    }
    parse_internal(sub_folder, deep + 1, mail_counter);
	m_owner->sendTag(StandardTag::TAG_CLOSE_FOLDER);
	}
	for (int i = 0; i < root.getMessageNumber(); ++i)
	{
    if (mail_counter >= MAILS_LIMIT)
    {
      break;
    }
		auto message = root.getMessage(i);

    auto html_text = message.getTextAsHtml();
    if(html_text)
    {
      auto callback = m_owner->sendTag(StandardTag::TAG_MAIL, "", {{"subject", message.getName()}, {"date", message.getCreationDate()}, {"level", deep}});
      if(callback.skip)
      {
        continue;
      }
      m_owner->sendTag(StandardTag::TAG_MAIL_BODY);
      parse_element(html_text->data(), html_text->size(), "html");
      ++mail_counter;
      m_owner->sendTag(StandardTag::TAG_CLOSE_MAIL_BODY);
    }

		auto attachments = message.getAttachments();
    for (auto &attachment : attachments)
    {
      std::string extension = attachment.m_name.substr(attachment.m_name.find_last_of(".") + 1);
      std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
      auto callback = m_owner->sendTag(
        StandardTag::TAG_ATTACHMENT, "", {{"name", attachment.m_name}, {"size", attachment.m_size}, {"extension", extension}});
      if(callback.skip)
      {
        continue;
      }
      parse_element((const char *) attachment.m_raw_data.get(), attachment.m_size, extension);
      m_owner->sendTag(StandardTag::TAG_CLOSE_ATTACHMENT);
    }
	m_owner->sendTag(StandardTag::TAG_CLOSE_MAIL);
	}
}

void PSTParser::Implementation::parse() const
{
	libpff_file_t* file = nullptr;
	pffError error{nullptr};
	if (libpff_file_initialize(&file, &error) != 1)
	{
		doctotext_log(doctotext::error) << "Unable to initialize file.";
		return;
	}

  libbfio_handle_t* handle = nullptr;
  auto time_id = std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count());
  std::string filename{time_id + "_temp.pst"};
  try
  {
    std::filesystem::path temp_file_path{std::filesystem::temp_directory_path().string() + "/" + time_id + "_temp.pst"};
    filename = temp_file_path.make_preferred().string();
  }
  catch (std::filesystem::filesystem_error &error)
  {
    doctotext_log(doctotext::error) << error.what();
  }

  if (!m_file_name.empty())
  {
    if (libpff_file_open(file, m_file_name.c_str(), LIBPFF_OPEN_READ, &error) != 1)
    {
      doctotext_log(doctotext::error) << "Unable to open file.";
      libpff_file_free(&file, NULL);
      return;
    }
  }
  else
  {
    libbfio_error_t* error = nullptr;
    auto filename_length = filename.size();
    libbfio_file_initialize(&handle, &error);
    libbfio_file_set_name(handle, filename.c_str(), filename_length, &error);
    libbfio_handle_open(handle, LIBBFIO_OPEN_READ_WRITE_TRUNCATE, &error);
    libbfio_handle_write_buffer(handle, (const uint8_t*)m_buffer, m_size, &error);
    libpff_file_open_file_io_handle(file, handle, LIBBFIO_OPEN_READ, &error);
  }

	pffItem root = NULL;
	libpff_file_get_root_folder(file, &root, &error);
	Folder root_folder(std::move(root));
  unsigned int mail_counter = 0;
  parse_internal(root_folder, 0, mail_counter);

  if (handle)
  {
    libpff_file_close(file, &error);
    libbfio_handle_close(handle, &error);
    std::remove(filename.c_str());
    handle = nullptr;
  }
}

void
PSTParser::parse() const
{
	doctotext_log(debug) << "Using PST parser.";
  impl->parse();
}

Parser &
PSTParser::withParameters(const doctotext::ParserParameters &parameters)
{
  Parser::withParameters(parameters);
  impl->addParameter(parameters);
  return *this;
}

PSTParser::PSTParser(const std::string& file_name, const std::shared_ptr<doctotext::ParserManager> &inParserManager)
: Parser(inParserManager)
{
  impl = new Implementation(file_name, this);
  impl->parserManager = inParserManager;
}

PSTParser::PSTParser(const char* buffer, size_t size, const std::shared_ptr<doctotext::ParserManager> &inParserManager)
: Parser(inParserManager)
{
  impl = new Implementation(buffer, size, this);
  impl->parserManager = inParserManager;
}

PSTParser::~PSTParser()
{
	delete impl;
}

bool
PSTParser::isPST() const
{
	pffError error;
	libpff_file_t* file = NULL;

	if (libpff_file_initialize(&file, &error) != 1)
	{
		return false;
	}

  libbfio_handle_t* handle = nullptr;
  auto time_id = std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count());
  std::string filename{time_id + "_temp.pst"};
  try
  {
    std::filesystem::path temp_file_path{std::filesystem::temp_directory_path().string() + "/" + time_id + "_temp.pst"};
    filename = temp_file_path.make_preferred().string();
  }
  catch (std::filesystem::filesystem_error &error)
  {
    doctotext_log(doctotext::error) << error.what();
  }

  auto filename_length = filename.size();
  libbfio_file_initialize(&handle, &error);
  libbfio_file_set_name(handle, filename.c_str(), filename_length, &error);
  libbfio_handle_open(handle, LIBBFIO_OPEN_READ_WRITE_TRUNCATE, &error);
  libbfio_handle_write_buffer(handle, (const uint8_t*)impl->m_buffer, impl->m_size, &error);
  libpff_file_open_file_io_handle(file, handle, LIBBFIO_OPEN_READ, &error);

  pffItem root = NULL;
  int result = libpff_file_get_root_folder(file, &root, &error);
  if (handle)
  {
    libpff_file_close(file, &error);
    libbfio_handle_close(handle, &error);
    std::remove(filename.c_str());
    handle = nullptr;
  }
  if (result != 1)
  {
    return false;
  }
  return true;
}
