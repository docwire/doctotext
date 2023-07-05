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

#include <numeric>
#include <fstream>

#include "chain_element.h"
#include "decompress_archives.h"
#include "input.h"
#include "parsing_chain.h"
#include "parser_manager.h"
#include "simple_extractor.h"

using namespace doctotext;

class SimpleExtractor::Implementation
{
public:
  Implementation(const std::string &file_name, const std::string &plugins_path)
    : m_parser_manager(new ParserManager(plugins_path)),
      m_file_name(file_name)
  {}

  Implementation(std::istream& input_stream, const std::string &plugins_path)
    : m_parser_manager(new ParserManager(plugins_path)),
      m_input_stream(&input_stream)
  {}

  template<typename ExtractorType>
  std::string getText() const
  {
    std::stringstream ss;
    ParsingChain chain = DecompressArchives() | Importer(m_parameters, m_parser_manager);

    if (!m_chain_elements.empty())
    {
      std::for_each(m_chain_elements.begin(), m_chain_elements.end(),
                    [&chain](const std::shared_ptr<ChainElement> &chainElement)
                    {
                      chain = chain | (*chainElement);
                    });
    }
    chain | ExtractorType(ss);
    if (!m_file_name.empty())
    {
      Input(m_file_name) | chain;
    }
    else if(m_input_stream)
    {
      Input(m_input_stream) | chain;
    }
    return ss.str();
  }

  template<typename ExtractorType>
  void parseText(std::ostream &out_stream) const
  {
    ParsingChain chain = DecompressArchives() | Importer(m_parameters, m_parser_manager);

    if (!m_chain_elements.empty())
    {
      std::for_each(m_chain_elements.begin(), m_chain_elements.end(),
                    [&chain](const std::shared_ptr<ChainElement> &chainElement)
                    {
                      chain = chain | (*chainElement);
                    });
    }
    chain | ExtractorType(out_stream);
    if (!m_file_name.empty())
    {
      Input(m_file_name) | chain;
    }
    else if(m_input_stream)
    {
      Input(m_input_stream) | chain;
    }
  }

  void
  addParameters(const doctotext::ParserParameters &parameters)
  {
    m_parameters += parameters;
  }

  void
  addChainElement(ChainElement *chainElement)
  {
    m_chain_elements.push_back(std::shared_ptr<ChainElement>(chainElement));
  }

  void
  setFormattingStyle(const FormattingStyle &style)
  {
    m_parameters += doctotext::ParserParameters("formatting_style", style);
  }

  void
  addCallbackFunction(NewNodeCallback new_code_callback)
  {
    addChainElement(new TransformerFunc(new_code_callback));
  }

  std::string
  getPlainText() const
  {
    return getText<PlainTextExporter>();
  }

  std::string
  getHtmlText() const
  {
    return getText<HtmlExporter>();
  }

  void parseAsPlainText(std::ostream &out_stream) const
  {
    parseText<PlainTextExporter>(out_stream);
  }

  void parseAsHtml(std::ostream &out_stream) const
  {
    parseText<HtmlExporter>(out_stream);
  }

  void parseAsCsv(std::ostream &out_stream) const
  {
    parseText<experimental::CsvExporter>(out_stream);
  }

  std::string
  getMetaData() const
  {
    return getText<MetaDataExporter>();
  }

  const std::shared_ptr<ParserManager> m_parser_manager;
  std::vector<std::shared_ptr<ChainElement>> m_chain_elements;
  std::string m_file_name;
  std::istream* m_input_stream;
  doctotext::ParserParameters m_parameters;
};

SimpleExtractor::SimpleExtractor(const std::string &file_name, const std::string &plugins_path)
{
  impl = std::unique_ptr<Implementation>{new Implementation{file_name, plugins_path}};
}

SimpleExtractor::SimpleExtractor(std::istream &input_stream, const std::string &plugins_path)
{
  impl = std::unique_ptr<Implementation>{new Implementation{input_stream, plugins_path}};
}

SimpleExtractor::~SimpleExtractor()
{
}

void
SimpleExtractor::addParameters(const doctotext::ParserParameters &parameters)
{
  impl->addParameters(parameters);
}

void
SimpleExtractor::addChainElement(ChainElement *chainElement)
{
  impl->addChainElement(chainElement);
}

void
SimpleExtractor::setFormattingStyle(const FormattingStyle &style)
{
  impl->setFormattingStyle(style);
}

void
SimpleExtractor::addCallbackFunction(NewNodeCallback new_code_callback)
{
  impl->addCallbackFunction(new_code_callback);
}

std::string
SimpleExtractor::getPlainText() const
{
  return impl->getPlainText();
}

std::string
SimpleExtractor::getHtmlText() const
{
  return impl->getHtmlText();
}

void
SimpleExtractor::parseAsPlainText(std::ostream &out_stream) const
{
  impl->parseAsPlainText(out_stream);
}

void
SimpleExtractor::parseAsHtml(std::ostream &out_stream) const
{
  impl->parseAsHtml(out_stream);
}

void
SimpleExtractor::parseAsCsv(std::ostream &out_stream) const
{
  impl->parseAsCsv(out_stream);
}

std::string
SimpleExtractor::getMetaData() const
{
  return impl->getMetaData();
}
