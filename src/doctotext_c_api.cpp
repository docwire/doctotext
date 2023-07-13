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

#include <cstring>
#include <cstdio>
#include <memory>
#include <iostream>
#include <sstream>
#include "parser.h"
#include "parser_manager.h"
#include "parser_parameters.h"
#include "doctotext_c_api.h"
#include "html_writer.h"

#include "input.h"
#include "importer.h"
#include "exporter.h"
#include "log.h"
#include "transformer_func.h"
#include "parsing_chain.h"
#include "simple_extractor.h"
#include "plain_text_writer.h"
#include "exception.h"

using namespace doctotext;

struct DocToTextParserManager
{
  std::shared_ptr<ParserManager> m_parser_manager;
};

struct DocToTextParser
{
  std::unique_ptr<Parser> m_parser;
};

struct DocToTextInfo
{
  Info* m_info;
};

struct DocToTextParameters
{
  ParserParameters m_parameters;
};

struct DocToTextWriter
{
  std::unique_ptr<Writer> m_writer;
};

struct DocToTextInput
{
  std::shared_ptr<InputBase> m_input;
  std::stringstream m_stream;
  FILE *m_file;
};

struct DocToTextImporter
{
  std::shared_ptr<Importer> m_importer;
};

struct DocToTextExporter
{
  std::shared_ptr<Exporter> m_exporter;
  std::stringstream m_stream;
  FILE *m_file;
};

struct DocToTextTransformer
{
  std::shared_ptr<TransformerFunc> m_transformer;
};

struct DocToTextParsingChain
{
  std::shared_ptr<ParsingChain> m_parsing_chain;
};

struct DocToTextSimpleExtractor
{
  std::unique_ptr<SimpleExtractor> m_simple_extractor;
};

DocToTextParserManager*
DOCTOTEXT_CALL doctotext_init_parser_manager(const char* path_to_plugins)
{
  auto parser_manager = new DocToTextParserManager;
  parser_manager->m_parser_manager = std::make_shared<ParserManager>(path_to_plugins);
  return parser_manager;
}

char**
DOCTOTEXT_CALL doctotext_parser_manager_get_available_formats(DocToTextParserManager* parser_manager, unsigned int *formats_number)
{
  auto formats = parser_manager->m_parser_manager->getAvailableExtensions();
  *formats_number = formats.size();
  auto c_formats = new char*[formats.size()];
  unsigned int index = 0;
  for (const auto &format : formats)
  {
    c_formats[index] = strdup(format.c_str());
    index++;
  }
  return c_formats;
}

DocToTextParser*
DOCTOTEXT_CALL doctotext_parser_manager_get_parser_by_extension(DocToTextParserManager* parser_manager, const char *path)
{
  auto parser_builder = parser_manager->m_parser_manager->findParserByExtension(path);
  if (parser_builder)
  {
    DocToTextParser* c_parser = new DocToTextParser;
    c_parser->m_parser = (*parser_builder)->withParserManager(parser_manager->m_parser_manager)
                                          .build(path);
    return c_parser;
  }
  return nullptr;
}

void
DOCTOTEXT_CALL doctotext_parser_parse(DocToTextParser* parser)
{
  try
  {
    parser->m_parser->parse();
  }
  catch (doctotext::Exception &e)
  {
    doctotext_log(error) << e.what();
  }
  catch(...)
  {
    doctotext_log(error) << "Cannot parse file: Unknown error";
  }
}

void
DOCTOTEXT_CALL doctotext_parser_add_callback_on_new_node(DocToTextParser* parser, void (*callback)(DocToTextInfo*, void* data), void* data)
{
  auto caller = [callback, data](Info& info){
    auto doc_to_text_info = new DocToTextInfo;
    auto info_parameter = new Info(info);
    doc_to_text_info->m_info = info_parameter;
    callback(doc_to_text_info, data);
    info.cancel = doc_to_text_info->m_info->cancel;
    info.skip = doc_to_text_info->m_info->skip;
  };
  parser->m_parser->addOnNewNodeCallback(caller);
}

void
DOCTOTEXT_CALL doctotext_parser_add_parameters(DocToTextParser* parser, DocToTextParameters* parameters)
{
  parser->m_parser->withParameters(parameters->m_parameters);
}

void
DOCTOTEXT_CALL doctotext_free_parser(DocToTextParser* parser)
{
  delete parser;
}

const char*
DOCTOTEXT_CALL doctotext_info_get_plain_text(DocToTextInfo* info)
{
  return info->m_info->plain_text.c_str();
}

const char*
DOCTOTEXT_CALL doctotext_info_get_tag_name(DocToTextInfo* info)
{
  return info->m_info->tag_name.c_str();
}

const char*
DOCTOTEXT_CALL doctotext_info_get_string_attribute(DocToTextInfo* info, const char* attribute_name)
{
  auto value = info->m_info->getAttributeValue<std::string>(attribute_name);
  return value ? strdup(value->c_str()) : nullptr;
}

unsigned int DOCTOTEXT_CALL doctotext_info_get_uint_attribute(DocToTextInfo* info, const char* attribute_name)
{
  auto value = info->m_info->getAttributeValue<uint32_t>(attribute_name);
  return value ? *value : 0;
}

void
DOCTOTEXT_CALL doctotext_info_set_skip(DocToTextInfo* info, bool skip)
{
  info->m_info->skip = skip;
}

void DOCTOTEXT_CALL
doctotext_info_set_cancel_parser(DocToTextInfo* info, bool cancel)
{
  info->m_info->cancel = cancel;
}

DocToTextParameters*
DOCTOTEXT_CALL doctotext_create_parameter()
{
  return new DocToTextParameters;
}

void
DOCTOTEXT_CALL doctotext_add_int_parameter(DocToTextParameters* parameters, const char* name, int value)
{
  parameters->m_parameters += ParserParameters(name, value);
}

void DOCTOTEXT_CALL doctotext_add_uint_parameter(DocToTextParameters* parameters, const char* name, unsigned int value)
{
  parameters->m_parameters += ParserParameters(name, value);
}

void
DOCTOTEXT_CALL doctotext_add_float_parameter(DocToTextParameters* parameters, const char* name, float value)
{
  parameters->m_parameters += ParserParameters(name, value);
}

void
DOCTOTEXT_CALL doctotext_add_string_parameter(DocToTextParameters* parameters, const char* name, const char* value)
{
  parameters->m_parameters += ParserParameters(name, value);
}

void
DOCTOTEXT_CALL doctotext_writer_write(DocToTextWriter* writer, DocToTextInfo* info, FILE* out_stream)
{
  std::stringstream stream;
  writer->m_writer->write_to(*info->m_info, stream);
  fprintf(out_stream, "%s", stream.str().c_str());
}

DocToTextWriter*
DOCTOTEXT_CALL doctotext_create_html_writer()
{
  auto writer = new DocToTextWriter;
  writer->m_writer = std::make_unique<HtmlWriter>();
  return writer;
}

DocToTextWriter*
DOCTOTEXT_CALL doctotext_create_plain_text_writer()
{
  auto writer = new DocToTextWriter;
  writer->m_writer = std::make_unique<PlainTextWriter>();
  return writer;
}

void
DOCTOTEXT_CALL doctotext_free_writer(DocToTextWriter* writer)
{
  writer->m_writer.reset();
}

DocToTextInput*
DOCTOTEXT_CALL doctotext_create_input_from_file_name(const char *file_name)
{
  auto input = new DocToTextInput;
  input->m_input = std::make_shared<InputBase>(file_name);
  return input;
}

DocToTextInput*
DOCTOTEXT_CALL doctotext_create_input_from_stream(FILE *input_stream)
{
  return nullptr;
}

DocToTextImporter*
DOCTOTEXT_CALL doctotext_create_importer(DocToTextParserManager *manager)
{
  auto importer = new DocToTextImporter;
  importer->m_importer = std::make_shared<Importer>(ParserParameters(), manager->m_parser_manager);
  return importer;
}

DocToTextExporter*
DOCTOTEXT_CALL doctotext_create_plain_text_exporter(FILE *output_stream)
{
  auto exporter = new DocToTextExporter;
  exporter->m_file = output_stream;
  exporter->m_exporter = std::make_shared<PlainTextExporter>(exporter->m_stream);
  return exporter;
}

DocToTextExporter*
DOCTOTEXT_CALL doctotext_create_html_exporter(FILE *output_stream)
{
  auto exporter = new DocToTextExporter;
  exporter->m_file = output_stream;
  exporter->m_exporter = std::make_shared<HtmlExporter>(exporter->m_stream);
  return exporter;
}

DocToTextTransformer*
DOCTOTEXT_CALL doctotext_create_transfomer(void (*callback)(DocToTextInfo*, void* data), void* data)
{
  auto transform_function = [callback, data](Info& info){
    auto doc_to_text_info = new DocToTextInfo;
    auto info_parameter = new Info(info);
    doc_to_text_info->m_info = info_parameter;
    callback(doc_to_text_info, data);
    info.cancel = doc_to_text_info->m_info->cancel;
    info.skip = doc_to_text_info->m_info->skip;
  };
  auto transformer = new DocToTextTransformer;
  transformer->m_transformer = std::make_shared<TransformerFunc>(transform_function);
  return transformer;
}

DocToTextParsingChain*
DOCTOTEXT_CALL doctotext_connect_input_to_importer(DocToTextInput* input, DocToTextImporter *importer)
{
  DocToTextParsingChain *chain = new DocToTextParsingChain;
  chain->m_parsing_chain = std::make_shared<ParsingChain>(*input->m_input, *importer->m_importer);
  return chain;
}

DocToTextParsingChain*
DOCTOTEXT_CALL doctotext_connect_importer_to_exporter(DocToTextImporter* importer, DocToTextExporter *exporter)
{
  DocToTextParsingChain *chain = new DocToTextParsingChain;
  chain->m_parsing_chain = std::make_shared<ParsingChain>(*importer->m_importer, *exporter->m_exporter);
  fprintf(exporter->m_file, "%s", exporter->m_stream.str().c_str());
  return chain;
}

void
DOCTOTEXT_CALL doctotext_parsing_chain_set_input(DocToTextParsingChain *parsing_chain, FILE* input_stream)
{

}

DocToTextParsingChain*
DOCTOTEXT_CALL doctotext_connect_importer_to_transformer(DocToTextImporter* importer, DocToTextTransformer *transformer)
{
  DocToTextParsingChain *chain = new DocToTextParsingChain;
  chain->m_parsing_chain = std::make_shared<ParsingChain>(*importer->m_importer | *transformer->m_transformer);
  return chain;
}

DocToTextParsingChain*
DOCTOTEXT_CALL doctotext_connect_parsing_chain_to_transformer(DocToTextParsingChain* parsing_chain, DocToTextTransformer *transformer)
{
  *(parsing_chain->m_parsing_chain) = *(parsing_chain->m_parsing_chain) | *(transformer->m_transformer);
  return parsing_chain;
}

DocToTextParsingChain*
DOCTOTEXT_CALL doctotext_connect_parsing_chain_to_exporter(DocToTextParsingChain* parsing_chain, DocToTextExporter *exporter)
{
  *(parsing_chain->m_parsing_chain) = *(parsing_chain->m_parsing_chain) | *(exporter->m_exporter);
  fprintf(exporter->m_file, "%s", exporter->m_stream.str().c_str());
  return parsing_chain;
}

void
DOCTOTEXT_CALL doctotext_free_input(DocToTextInput *input)
{
  delete input;
  input = nullptr;
}

void
DOCTOTEXT_CALL doctotext_free_importer(DocToTextImporter *importer)
{
  delete importer;
  importer = nullptr;
}
void
DOCTOTEXT_CALL doctotext_free_exporter(DocToTextExporter *exporter)
{
  delete exporter;
  exporter = nullptr;
}
void
DOCTOTEXT_CALL doctotext_free_transformer(DocToTextTransformer *transformer)
{
  delete transformer;
  transformer = nullptr;
}
void
DOCTOTEXT_CALL doctotext_free_parsing_chain(DocToTextParsingChain *parsing_chain)
{
  delete parsing_chain;
  parsing_chain = nullptr;
}

DocToTextSimpleExtractor*
DOCTOTEXT_CALL doctotext_create_simple_extractor(const char* file_name)
{
  auto extractor = new DocToTextSimpleExtractor;
  extractor->m_simple_extractor = std::make_unique<SimpleExtractor>(file_name);
  return extractor;
}

const char *
DOCTOTEXT_CALL doctotext_simple_extractor_get_plain_text(DocToTextSimpleExtractor* extractor)
{
  return strdup(extractor->m_simple_extractor->getPlainText().c_str());
}

void
DOCTOTEXT_CALL doctotext_simple_extractor_add_callback_function(DocToTextSimpleExtractor* extractor, void (*callback)(DocToTextInfo*, void* data), void* data)
{
  auto callback_function = [callback, data](Info& info){
    auto doc_to_text_info = new DocToTextInfo;
    auto info_parameter = new Info(info);
    doc_to_text_info->m_info = info_parameter;
    callback(doc_to_text_info, data);
    info.cancel = doc_to_text_info->m_info->cancel;
    info.skip = doc_to_text_info->m_info->skip;
  };
  extractor->m_simple_extractor->addCallbackFunction(callback_function);
}
