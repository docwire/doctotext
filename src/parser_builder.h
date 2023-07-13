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


#ifndef PARSER_BUILDER_H
#define PARSER_BUILDER_H

#include <vector>
#include <memory>
#include <functional>
#include "parser.h"
#include "parser_manager.h"
#include "parser_parameters.h"
#include "defines.h"

namespace doctotext
{
/**
 * Abstract class to build parsers. Parser could be built from path to file or from data buffer.
 */
class DllExport ParserBuilder
{
public:
  /**
   * @brief Builds new parser object.
   * @param inFileName path to file
   * @return pointer to new parser object
   */
  virtual std::unique_ptr<Parser> build(const std::string &inFileName) const = 0;

  /**
   *
   * @brief Builds new parser object.
   * @param buffer raw data of file to be parsed
   * @param size file size
   * @return pointer to new parser object
   */
  virtual std::unique_ptr<Parser> build(const char *buffer, size_t size) const = 0;

  /**
   * @brief Adds callback function.
   * @param callbacks
   */
  virtual ParserBuilder &withOnNewNodeCallbacks(const std::vector<NewNodeCallback> &callbacks) = 0;

  /**
   * @brief Sets parser manager.
   * @param inParserManager
   */
  virtual ParserBuilder &withParserManager(const std::shared_ptr<ParserManager> &inParserManager) = 0;

  /**
   * @brief Sets parser parameters.
   * @param inParameters
   */
  virtual ParserBuilder &withParameters(const ParserParameters &inParameters) = 0;

  virtual ~ParserBuilder() = default;
};
} // namespace doctotext
#endif //PARSER_BUILDER_H
