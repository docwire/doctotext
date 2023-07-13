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

#include "ocr_parser.h"
#include "parser_parameters.h"

#include <tesseract/baseapi.h>
#include <tesseract/ocrclass.h>
#include <leptonica/allheaders.h>

#include <boost/signals2.hpp>

#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include "exception.h"
#include "log.h"
#include "misc.h"

struct OCRParser::Implementation
{
    explicit Implementation(const std::string& file_name)
    : m_file_name{ file_name }, m_buffer{nullptr}, m_buffer_size{0}
    { 
    }

  Implementation(const char* buffer, size_t size)
    : m_buffer{buffer}, m_buffer_size{size}
  {
  }

    ~Implementation() = default;

    std::string m_file_name{};
    const char* m_buffer;
    size_t m_buffer_size;
    std::string m_tessdata_prefix;
    boost::signals2::signal<void(doctotext::Info &info)> m_on_new_node_signal;
};  

void OCRParser::ImplementationDeleter::operator() (Implementation* impl)
{
    delete impl;
}

std::string OCRParser::get_default_tessdata_prefix()
{
    return "./tessdata/";
}

OCRParser::OCRParser(const OCRParser& ocr_parser)
{
  if (!ocr_parser.impl->m_file_name.empty())
  {
    impl = std::unique_ptr<Implementation, ImplementationDeleter>
      {new Implementation{ ocr_parser.impl->m_file_name }, ImplementationDeleter{}};
  }
  else
  {
    impl = std::unique_ptr<Implementation, ImplementationDeleter>
      {new Implementation{ocr_parser.impl->m_buffer, ocr_parser.impl->m_buffer_size}, ImplementationDeleter{}};
  }
}

OCRParser::OCRParser(const std::string& file_name, const std::shared_ptr<doctotext::ParserManager> &inParserManager)
: Parser(inParserManager)
{
  impl = std::unique_ptr<Implementation, ImplementationDeleter>{new Implementation{file_name}, ImplementationDeleter{}};
}

OCRParser::OCRParser(const char* buffer, size_t size, const std::shared_ptr<doctotext::ParserManager> &inParserManager)
: Parser(inParserManager)
{
  impl = std::unique_ptr<Implementation, ImplementationDeleter> {new Implementation{buffer, size}, ImplementationDeleter{}};
}

OCRParser::~OCRParser() = default;

using namespace doctotext;

Pix* pixToGrayscale(Pix* pix)
{
    Pix* output{};
    switch(pix->d)
    {
    case 8:
        output = pixRemoveColormap(pix, REMOVE_CMAP_TO_GRAYSCALE); 
        break;
    case 16:
        {
            auto tmp = pixConvert16To8(pix, 0);
            output = pixRemoveColormap(tmp, REMOVE_CMAP_TO_GRAYSCALE); 
            pixDestroy(&tmp);
            break;
        }
    case 32:
        {
            auto tmp = pixRemoveAlpha(pix);
            output = pixConvertRGBToGrayFast(tmp);
            pixDestroy(&tmp);
            break;	
        }
    default:
        throw std::runtime_error{ "Format not supported: bpp" + pix->d };
    }
    return output;
}

Pix* binarizePix(Pix* pix)
{
    Pix* temp{ NULL };
    // 200x200 - size of the binarized tiles; 0, 0 - no smoothing; 0.1 - typical scorefract
    pixOtsuAdaptiveThreshold(pix, 200, 200, 0, 0, 0.1, NULL, &temp);
    return temp;
}

Pix* scalePix(Pix* pix)
{
    Pix* scaled{ nullptr };
    if(pix->xres && pix->yres)
    {
        // Calculate the scale-factors
        constexpr float desired{ 301.f };
        float xScaleFactor{}, yScaleFactor{};

        xScaleFactor = desired / pix->xres;
        yScaleFactor = desired / pix->yres;

        scaled = pixScaleGrayLI(pix, xScaleFactor, yScaleFactor);
    }
    else
    {
        // For corrupted files
        scaled = pixScale(pix, 4, 4);
    }
    return scaled;
}

namespace
{
    using tesseract::TessBaseAPI;
    auto tessAPIDeleter = [](TessBaseAPI* tessAPI)
    {
        tessAPI->End();
        delete tessAPI;
    };
    using tessAPIWrapper = std::unique_ptr<TessBaseAPI, decltype(tessAPIDeleter)>;

    auto pixDeleter = [](Pix* pix)
    {
        pixDestroy(&pix);
    };
    using PixWrapper = std::unique_ptr<Pix, decltype(pixDeleter)>;
}

bool cancel (void* data, int words)
{
  auto* signal = reinterpret_cast<boost::signals2::signal<void(doctotext::Info &info)>*>(data);
  Info info;
  (*signal)(info);
  return info.cancel;
}

std::string OCRParser::plainText(const doctotext::FormattingStyle& formatting, const Language lang) const
{
    tessAPIWrapper api{ nullptr, tessAPIDeleter };
    try
    {
        api.reset(new TessBaseAPI{});
    }
    catch(const std::bad_alloc& exc)
    {
        throw;
    }

    auto tess_data_prefix = m_parameters.getParameterValue<std::string>("TESSDATA_PREFIX");
    if (tess_data_prefix)
    {
      impl->m_tessdata_prefix = *tess_data_prefix;
    }
    else
    {
      impl->m_tessdata_prefix = (std::filesystem::path(get_self_path()) / "tessdata").string();
    }

    if (api->Init(impl->m_tessdata_prefix.c_str(), doctotext::languageToName(lang).c_str())) {
        throw doctotext::Exception{ "Could not initialize tesseract.\n" };
    }

    // Read the image and convert to a gray-scale image
    PixWrapper gray{ nullptr, pixDeleter };

    PixWrapper image(impl->m_file_name.empty() ? pixReadMem((const unsigned char*)(impl->m_buffer), impl->m_buffer_size)
                     : pixRead(impl->m_file_name.c_str()), pixDeleter);


    PixWrapper inverted{ nullptr, pixDeleter };
    try
    {
        gray.reset(pixToGrayscale(image.get()));
	    NUMA* histogram = pixGetGrayHistogram(gray.get(), 1);

        double weight_sum{ 0 };
        double sum{ 0 };

        constexpr int shadeScale{ 256 }; // where 0 - black, 255 - white
        for(int i{ 0 }; i < shadeScale; ++i)
        {
            // calculating weighted average of shade
            sum += (i+1) * histogram->array[i];
            weight_sum += histogram->array[i];
        }

        if(static_cast<int>(sum / weight_sum) <= shadeScale / 2)
        {
            inverted.reset(pixInvert(nullptr, gray.get()));
        }
        else
        {
            inverted.reset(gray.release());
        }
    }
    catch(const std::exception& e)
    {
        throw;
    }

    api->SetImage(inverted.get());
    tesseract::ETEXT_DESC monitor;
    monitor.set_deadline_msecs(TIMEOUT);
    monitor.cancel = &cancel;
    monitor.cancel_this = &(impl->m_on_new_node_signal);
    api->Recognize(&monitor);
    auto txt = api->GetUTF8Text();
    std::string output{ txt };
    delete[] txt;
    return output;
}

void OCRParser::setTessdataPrefix(const std::string& tessdata_prefix)
{
    impl->m_tessdata_prefix = tessdata_prefix;
}

bool OCRParser::isOCR() const
{
    if(impl == nullptr) return false;
    Pix* image = impl->m_file_name.empty() ? pixReadMem((const unsigned char*)(impl->m_buffer), impl->m_buffer_size) :
                 pixRead(impl->m_file_name.c_str());
    return image != nullptr;
}

Parser&
OCRParser::withParameters(const doctotext::ParserParameters &parameters)
{
	doctotext::Parser::withParameters(parameters);
    return *this;
}

void
OCRParser::parse() const
{
  doctotext_log(debug) << "Using OCR parser.";
  Info info(StandardTag::TAG_TEXT);
  auto language = m_parameters.getParameterValue<Language>("language");
  info.plain_text = plainText(getFormattingStyle(), language ? *language : Language::english);
  impl->m_on_new_node_signal(info);
}

doctotext::Parser&
OCRParser::addOnNewNodeCallback(doctotext::NewNodeCallback callback)
{
  impl->m_on_new_node_signal.connect(callback);
  return *this;
}
