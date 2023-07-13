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
#include <filesystem>
#include <boost/signals2.hpp>

#include "exception.h"
#include "importer.h"
#include "log.h"

using namespace doctotext;

class Importer::Implementation
{
public:
  Implementation(const std::shared_ptr<ParserManager> &parser_manager,
                 const ParserParameters &parameters, Importer& owner)
    : m_parser_manager(parser_manager),
      m_parameters(parameters),
      m_owner(owner)
  {}

  Implementation(const Implementation &other, Importer& owner)
    : m_parser_manager(other.m_parser_manager),
      m_parameters(other.m_parameters),
      m_owner(owner)
  {}

  Implementation(const Implementation &&other, Importer& owner)
    : m_parser_manager(other.m_parser_manager),
      m_parameters(other.m_parameters),
      m_owner(owner)
  {}

  bool load_file_to_buffer(const std::string &file_name, std::vector<char> &buffer) const
  {
    std::ifstream src(file_name.c_str(), std::ios_base::in|std::ios_base::binary);
    if (!src.is_open())
    {
      return false;
    }
    buffer = std::vector<char>(std::istreambuf_iterator<char>(src), std::istreambuf_iterator<char>());
    return true;
  }

  bool
  isReadable(const std::filesystem::path& p) const
  {
    std::error_code ec;
    auto perms = std::filesystem::status(p, ec).permissions();
    if ((perms & std::filesystem::perms::owner_read) != std::filesystem::perms::none &&
        (perms & std::filesystem::perms::group_read) != std::filesystem::perms::none &&
        (perms & std::filesystem::perms::others_read) != std::filesystem::perms::none
            )
    {
      return true;
    }
    return false;
  }

  void
  process(Info& info)
  {
    if (info.tag_name != StandardTag::TAG_FILE)
    {
      m_owner.emit(info);
      return;
    }
    Info new_doc(StandardTag::TAG_DOCUMENT);
    m_owner.emit(new_doc);
    std::shared_ptr<ParserBuilder> builder;
    std::vector<char> buffer;
    std::istream* input_stream = nullptr;
    std::string file_path;
    if (info.getAttributeValue<std::istream*>("stream"))
    {
      input_stream = *info.getAttributeValue<std::istream*>("stream");
    }
    else if(info.getAttributeValue<std::string>("path"))
    {
      file_path = *info.getAttributeValue<std::string>("path");
    }
    if (!file_path.empty())
    {
      std::filesystem::path path{file_path};
      if (std::filesystem::exists(path))
      {
        if (isReadable(path))
        {
          builder = std::shared_ptr<ParserBuilder>(m_parser_manager->findParserByExtension(file_path).value_or(nullptr));
        }
        else
        {
          throw doctotext::Exception("file " + file_path + " is not readable");
        }
      }
      else
      {
        throw doctotext::Exception("file " + file_path + "  doesn't exist");
      }
    }
    else if(input_stream)
    {
      buffer = std::vector<char>((std::istreambuf_iterator<char>(*input_stream)), std::istreambuf_iterator<char>());
      builder = std::shared_ptr<ParserBuilder>(m_parser_manager->findParserByData(buffer).value_or(nullptr));
    }
    if (builder)
    {
      auto &builder_ref = builder->withOnNewNodeCallbacks({[this](doctotext::Info &info){ m_owner.emit(info);}})
        .withParserManager(m_parser_manager)
        .withParameters(m_parameters);

      if (!file_path.empty())
      {
        try
        {
          builder_ref.build(file_path)->parse();
        }
        catch (doctotext::EncryptedFileException &ex)
        {
          throw ex;
        }
        catch (doctotext::Exception &ex)
        {
          doctotext_log(doctotext::info) << "It is possible that wrong parser was selected. Trying different parsers." << std::endl;
          std::vector<char> buffer;
          load_file_to_buffer(file_path, buffer);
          auto second_builder = m_parser_manager->findParserByData(buffer);
          if (!second_builder)
          {
            ex.appendError("Error parsing file: " + file_path  + ". Tried different parsers, but file could not be recognized as another format. File may be corrupted or encrypted");
            throw ex;
          }
          std::shared_ptr<ParserBuilder>(*second_builder)
                  ->withOnNewNodeCallbacks({[this](doctotext::Info &info){ m_owner.emit(info);}})
                  .withParserManager(m_parser_manager)
                  .withParameters(m_parameters)
                  .build(buffer.data(), buffer.size())->parse();
        }
      }
      else
      {
        builder_ref.build(buffer.data(), buffer.size())->parse();
      }
    }
    else
    {
      throw doctotext::Exception("File format was not recognized.");
    }
    Info end_doc(StandardTag::TAG_CLOSE_DOCUMENT);
    m_owner.emit(end_doc);
  }

  void
  add_parameters(const ParserParameters &parameters)
  {
    m_parameters += parameters;
  }

  std::shared_ptr<ParserManager> m_parser_manager;
  std::shared_ptr<ParserBuilder> m_parser_builder;

  ParserParameters m_parameters;
  Importer& m_owner;
};

Importer::Importer(const ParserParameters &parameters, const std::shared_ptr<ParserManager> &parser_manager)
{
  impl = std::unique_ptr<Implementation>{new Implementation{parser_manager, parameters, *this}};
}

Importer::Importer(const Importer &other)
  :  ChainElement(other),
     impl(new Implementation(*(other.impl), *this))
{}

Importer::Importer(const Importer &&other)
  :  ChainElement(other),
     impl(new Implementation(*(other.impl), *this))
{}

Importer::~Importer()
{
}

Importer&
Importer::operator=(const Importer &other)
{
  impl.reset(new Implementation(*(other.impl), *this));
  return *this;
}

Importer&
Importer::operator=(const Importer &&other)
{
  impl.reset(new Implementation(*(other.impl), *this));
  return *this;
}

void
Importer::process(Info& info) const
{
  impl->process(info);
}

Importer*
Importer::clone() const
{
  return new Importer(*this);
}

void
Importer::add_parameters(const ParserParameters &parameters)
{
  impl->add_parameters(parameters);
}
