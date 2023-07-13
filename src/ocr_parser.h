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

#ifndef OCR_PARSER_HPP
#define OCR_PARSER_HPP

#include <string>
#include <memory>
#include <iosfwd>


#include "formatting_style.h"
#include "parser.h"
#include "parser_builder.h"

class OCRParser : public doctotext::Parser
{
private:
    struct Implementation;
    struct ImplementationDeleter { void operator() (Implementation*); };
    std::unique_ptr<Implementation, ImplementationDeleter> impl;
    const unsigned int TIMEOUT = 30000;

public:
    static std::string get_default_tessdata_prefix();

    OCRParser(const OCRParser& ocr_parser);
    OCRParser(const std::string& file_name, const std::shared_ptr<doctotext::ParserManager> &inParserManager = nullptr);
    OCRParser(const char* buffer, size_t size, const std::shared_ptr<doctotext::ParserManager> &inParserManager = nullptr);
    ~OCRParser();

    void parse() const override;
    Parser& addOnNewNodeCallback(doctotext::NewNodeCallback callback) override;
    static std::vector <std::string> getExtensions() {return {"tiff", "jpeg", "bmp", "png", "pnm", "jfif", "jpg", "webp"};}
    Parser& withParameters(const doctotext::ParserParameters &parameters) override;

    void setTessdataPrefix(const std::string& tessdata_prefix);
    bool isOCR() const;
    std::string plainText(const doctotext::FormattingStyle& formatting, const doctotext::Language lang) const;
};

#endif // OCR_PARSER_HPP
