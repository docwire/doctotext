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
#include <numeric>
#include <filesystem>
#include <boost/dll/import.hpp>
#include <iostream>

#include "log.h"
#include "misc.h"
#include "parser_manager.h"
#include "parser_provider.h"
#include "parser_builder.h"
#include "pthread.h"

using namespace doctotext;

static pthread_mutex_t load_providers_mutex = PTHREAD_MUTEX_INITIALIZER;

static std::set<std::string> correct_extensions = {".dll", ".so", ".dylib"};

class ParserManager::Implementation
{
public:
  explicit Implementation(const std::string &plugins_directory)
  {
    m_plugins_directory = plugins_directory.empty() ?
                          (std::filesystem::path(get_self_path()) / DEFAULT_PLUGINS_SUFFIX).string() : plugins_directory;
    pthread_mutex_lock(&load_providers_mutex);
    loadProviders();
    pthread_mutex_unlock(&load_providers_mutex);
  }

  void loadProviders()
  {
    providers.clear();
    const std::filesystem::path root{m_plugins_directory};
    if (std::filesystem::is_directory(root))
    {
      for (auto &dir_entry : std::filesystem::directory_iterator{root})
      {
          if (dir_entry.is_regular_file() && correct_extensions.find(dir_entry.path().extension().u8string()) != correct_extensions.end())
          {
            try
            {
              boost::shared_ptr<doctotext::ParserProvider>plugin_provider =
              boost::dll::import_symbol<doctotext::ParserProvider>(dir_entry.path().c_str(),
                                                              "plugin_parser_provider",
                                                              boost::dll::load_mode::append_decorations);
              providers.push_back(plugin_provider);
            }
            catch (const boost::system::system_error &e)
            {
              doctotext_log(error) << "Error loading plugin: " << e.what();
            }
          }
      }
    }
  }
  const std::string DEFAULT_PLUGINS_SUFFIX{"plugins"};
  std::vector<boost::shared_ptr<ParserProvider>> providers;
  std::string m_plugins_directory;
};

ParserManager::ParserManager()
{
  impl = std::make_unique<Implementation>("");
}

ParserManager::ParserManager(const std::string &plugins_directory)
{
  impl = std::make_unique<Implementation>(plugins_directory);
}

ParserManager::~ParserManager()
{
}

std::optional<ParserBuilder*>
ParserManager::findParserByExtension(const std::string &file_name) const
{
  std::string extension = file_name.substr(file_name.find_last_of(".") + 1);
  std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
  for (const auto &provider : impl->providers)
  {
    auto builder = provider->findParserByExtension(extension);
    if (builder)
    {
      return builder;
    }
  }
  return std::nullopt;
}

std::optional<ParserBuilder*>
ParserManager::findParserByData(const std::vector<char> &buffer) const
{
  for (const auto &provider : impl->providers)
  {
    auto builder = provider->findParserByData(buffer);
    if (builder)
    {
      return builder;
    }
  }
  return std::nullopt;
}

std::set<std::string>
ParserManager::getAvailableExtensions() const
{
  std::set<std::string> all_extensions;
  for (const auto &provider : impl->providers)
  {
    auto extensions = provider->getAvailableExtensions();
    std::copy(extensions.begin(), extensions.end(), std::inserter(all_extensions, all_extensions.end()));
  }
  return all_extensions;
}
