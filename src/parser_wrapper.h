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

#ifndef PARSERWRAPPER_H
#define PARSERWRAPPER_H

#include <iostream>

#include "metadata.h"
#include "formatting_style.h"
#include "parser.h"
#include "parser_builder.h"
#include "defines.h"

namespace doctotext
{

template<typename ParserType>
class DllExport ParserWrapper : public doctotext::Parser
{
public:
  explicit ParserWrapper(const std::string& file_name, const std::shared_ptr<doctotext::ParserManager> &inParserManager = nullptr)
  : Parser(inParserManager),
    m_parser(ParserType(file_name))
  {}

  ParserWrapper(const char* buffer, size_t size, const std::shared_ptr<doctotext::ParserManager> &inParserManager = nullptr)
  : Parser(inParserManager) ,
    m_parser(ParserType(buffer, size))
  {}

  void parse() const override
  {
    sendTag(doctotext::StandardTag::TAG_TEXT,  m_parser.plainText(getFormattingStyle()));
    sendTag(doctotext::StandardTag::TAG_METADATA, "", m_parser.metaData().getFieldsAsAny());
  }

  Parser &withParameters(const ParserParameters &parameters) override
  {
    doctotext::Parser::withParameters(parameters);
    return *this;
  }

  void setParserManager(const std::shared_ptr<doctotext::ParserManager> &inParserManager)
  {
    m_parser_manager = inParserManager;
  }

private:
  mutable ParserType m_parser;
};

template<typename ParserType>
class DllExport wrapper_parser_creator
{
public:
  static std::unique_ptr<doctotext::Parser>
  create(const std::string &inFileName, const std::shared_ptr<doctotext::ParserManager> &parserManager)
  {
    return std::make_unique<ParserWrapper<ParserType>>(inFileName, parserManager);
  }

  static std::unique_ptr<doctotext::Parser>
  create(const char* buffer, size_t size, const std::shared_ptr<doctotext::ParserManager> &parserManager)
  {
    return std::make_unique<ParserWrapper<ParserType>>(buffer, size, parserManager);
  }
};

template<typename ParserType>
class DllExport parser_creator
{
public:
  static std::unique_ptr<doctotext::Parser>
  create(const std::string &inFileName, const std::shared_ptr<doctotext::ParserManager> &parserManager)
  {
    return std::make_unique<ParserType>(inFileName, parserManager);
  }

  static std::unique_ptr<doctotext::Parser>
  create(const char* buffer, size_t size, const std::shared_ptr<doctotext::ParserManager> &parserManager)
  {
    return std::make_unique<ParserType>(buffer, size, parserManager);
  }
};

/**
 * @brief Provides the basic mechanism to build any parser.
 * @tparam ParserCreator type of parser to build
 */
template<typename ParserCreator>
class DllExport ParserBuilderWrapper : public doctotext::ParserBuilder
{
public:
  ParserBuilderWrapper()
  : m_parser_creator(ParserCreator())
  {

  }

  std::unique_ptr<doctotext::Parser>
  build(const std::string &inFileName) const override
  {
    auto parser = m_parser_creator.create(inFileName, m_parser_manager);
    for (auto &callback : m_callbacks)
    {
      parser->addOnNewNodeCallback(callback);
    }
    parser->withParameters(m_parameters);
    return parser;
  }

  std::unique_ptr<doctotext::Parser>
  build(const char* buffer, size_t size) const override
  {
    auto parser =  m_parser_creator.create(buffer, size, m_parser_manager);
    for (auto &callback : m_callbacks)
    {
      parser->addOnNewNodeCallback(callback);
    }
    parser->withParameters(m_parameters);
    return parser;
  }

  doctotext::ParserBuilder&
  withOnNewNodeCallbacks(const std::vector<doctotext::NewNodeCallback> &callbacks) override
  {
    m_callbacks = callbacks;
    return *this;
  }

  doctotext::ParserBuilder&
  withParserManager(const std::shared_ptr<doctotext::ParserManager> &inParserManager) override
  {
    m_parser_manager = inParserManager;
    return *this;
  }

  doctotext::ParserBuilder&
  withParameters(const ParserParameters &inParameter) override
  {
    m_parameters += inParameter;
    return *this;
  }

private:
  ParserCreator m_parser_creator;
  std::vector<doctotext::NewNodeCallback> m_callbacks;
  std::shared_ptr<doctotext::ParserManager> m_parser_manager;
  ParserParameters m_parameters;
};
} // namespace doctotext
#endif //PARSERWRAPPER_H
