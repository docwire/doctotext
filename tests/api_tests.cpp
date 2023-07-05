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

#include "gtest/gtest.h"
#include "../src/exception.h"
#include <pthread.h>
#include <string_view>
#include <tuple>
#include <fstream>
#include <iterator>
#include <array>
#include "../src/simple_extractor.h"
#include "../src/standard_filter.h"
#include <optional>
#include <algorithm>
#include "pthread.h"
#include "input.h"

void dots_to_underscores(std::string& str)
{
    std::transform(str.cbegin(), str.cend(), str.begin(), [](const auto ch)
        {   if(ch == '.') return '_'; 
            else return ch; 
        }
    );
}



class DocumentTests :public ::testing::TestWithParam<std::tuple<int, int, const char*, std::optional<doctotext::FormattingStyle>>> {
protected:
    static constexpr std::string_view test_directory{ "../../tests/" };
    doctotext::ParserParameters parameters{};

    void SetUp() override
    {
        doctotext::FormattingStyle style{};
        style.list_style.setPrefix(" * ");
        parameters += doctotext::ParserParameters{ "formatting_style", style };
  }

};

TEST_P(DocumentTests, SimpleExtractorTest)
{
    const auto [lower, upper, format, style] = GetParam();

    for(int i = lower; i <= upper; ++i)
    {
        // GIVEN
        std::string file_name{ test_directory.data() + std::to_string(i) + "." + format };

        std::ifstream ifs{ file_name + ".out" };
        ASSERT_TRUE(ifs.good()) <<  "File " << file_name << ".out" << " not found\n";
        
        std::string expected_text{ std::istreambuf_iterator<char>{ifs},
            std::istreambuf_iterator<char>{}};

        SCOPED_TRACE("file_name = " + file_name);

        // WHEN
        doctotext::SimpleExtractor simple_extractor{ file_name, "../plugins" }; // create a simple extractor

        simple_extractor.addParameters(parameters);

        std::string parsed_text{ simple_extractor.getPlainText() };

        // THEN
        EXPECT_EQ(expected_text, parsed_text);
    }
}

TEST_P(DocumentTests, ReadFromBufferTest)
{
    const auto [lower, upper, format, style] = GetParam();

    for(int i = lower; i <= upper; ++i)
    {
        // GIVEN
        std::string file_name{ test_directory.data() + std::to_string(i) + "." + format };

        std::ifstream ifs{ file_name + ".out" };
        ASSERT_TRUE(ifs.good()) <<  "File " << file_name << ".out" << " not found\n";
        
        std::string expected_text{ std::istreambuf_iterator<char>{ifs},
            std::istreambuf_iterator<char>{}};

        SCOPED_TRACE("file_name = " + file_name);

        std::ifstream ifs_input{ file_name, std::ios_base::binary };

        // WHEN
        auto parser_manager = std::make_shared<doctotext::ParserManager>("../plugins"); // create parser manager
        std::stringstream output_stream{};

        doctotext::Input(&ifs_input) |
          doctotext::Importer(parameters, parser_manager) |
          doctotext::PlainTextExporter(output_stream);

        std::string parsed_text{ std::istreambuf_iterator<char>{output_stream},
            std::istreambuf_iterator<char>{}};

        // THEN
        EXPECT_EQ(expected_text, parsed_text);
    }
}

INSTANTIATE_TEST_SUITE_P(
    BasicTests, DocumentTests,
    ::testing::Values(
        std::make_tuple(1, 9, "odt", std::nullopt),
        std::make_tuple(1, 9, "fodt", std::nullopt),
        std::make_tuple(1, 9, "ods", std::nullopt),
        std::make_tuple(1, 9, "fods", std::nullopt),
        std::make_tuple(1, 9, "odp", std::nullopt),
        std::make_tuple(1, 9, "fodp", std::nullopt),
        std::make_tuple(1, 9, "odg", std::nullopt),
        std::make_tuple(1, 9, "fodg", std::nullopt),
        std::make_tuple(1, 9, "rtf", std::nullopt),
        std::make_tuple(1, 9, "doc", std::nullopt),
        std::make_tuple(1, 9, "xls", std::nullopt),
        std::make_tuple(1, 9, "xlsb", std::nullopt),
        std::make_tuple(1, 9, "pdf", std::nullopt),
        std::make_tuple(1, 9, "ppt", std::nullopt),
        std::make_tuple(1, 9, "docx", std::nullopt),
        std::make_tuple(1, 9, "xlsx", std::nullopt),
        std::make_tuple(1, 9, "pptx", std::nullopt),
        std::make_tuple(1, 9, "pages", std::nullopt),
        std::make_tuple(1, 9, "numbers", std::nullopt),
        std::make_tuple(1, 9, "key", std::nullopt),
        std::make_tuple(1, 9, "html", std::nullopt),
        std::make_tuple(1, 1, "pst", std::nullopt)
                      ),
    [](const ::testing::TestParamInfo<DocumentTests::ParamType>& info) {
      std::string name = std::string{ std::get<2>(info.param) } + "_basic_tests";
      return name;
    });

class MetadataTest : public ::testing::TestWithParam<const char*>
{
protected:
    static constexpr std::string_view test_directory{ "../../tests/" };

    static constexpr std::array<std::string_view, 2> names
    {
        "meta_libreoffice_3.5_created", 
        "meta_libreoffice_3.5_modified" 
    };
};

TEST_P(MetadataTest, SimpleExtractorTest)
{
    auto format = GetParam();

    for(auto name : names)
    {
        // GIVEN
        std::string file_name{ std::string{ test_directory.data() } + name.data() + "." + format };

        std::ifstream ifs{ file_name + ".out" };
        ASSERT_TRUE(ifs.good()) <<  "File " << file_name << ".out" << " not found\n";
        
        std::string expected_text{ std::istreambuf_iterator<char>{ifs},
            std::istreambuf_iterator<char>{}};

        SCOPED_TRACE("file_name = " + file_name);

        // WHEN
        doctotext::SimpleExtractor extractor{ file_name, "../plugins" };
        std::string parsed_text{ extractor.getMetaData() };

        // THEN
        EXPECT_EQ(expected_text, parsed_text);
    }
}

INSTANTIATE_TEST_SUITE_P(
    SimpleExtractorMetadataTests, MetadataTest,
    ::testing::Values(
        "odt", "ods", "odp", "odg", "rtf", "doc", "xls", "ppt", "docx", "xlsx", "pptx", "html"
                      ),
    [](const ::testing::TestParamInfo<MetadataTest::ParamType>& info) {
      std::string name = std::string{ info.param } + "_basic_metadata_tests";
      return name;
    });

class CallbackTest : public ::testing::TestWithParam<std::tuple<const char*, const char*, doctotext::NewNodeCallback>>
{
protected:
    static constexpr std::string_view test_directory{ "../../tests/" };
    doctotext::ParserParameters parameters{};

    void SetUp() override
    {
        doctotext::FormattingStyle style{};
        style.list_style.setPrefix(" * ");
        parameters += doctotext::ParserParameters{ "formatting_style", style };
  }
};


TEST_P(CallbackTest, SimpleExtractorTest)
{
    const auto [name, out_name, callback] = GetParam();

    // GIVEN
    std::string file_name{ std::string{ test_directory.data() } + name };
    std::string output_name{ std::string{ test_directory.data() } + out_name };

    std::ifstream ifs{ output_name };
    ASSERT_TRUE(ifs.good()) <<  "File " << file_name << ".out" << " not found\n";
    
    std::string expected_text{ std::istreambuf_iterator<char>{ifs},
        std::istreambuf_iterator<char>{}};

    SCOPED_TRACE("file_name = " + file_name);

    // WHEN
    doctotext::SimpleExtractor simple_extractor{ file_name, "../plugins" }; // create a simple extractor

    simple_extractor.addCallbackFunction(callback);
    simple_extractor.addParameters(parameters);

    std::string parsed_text{ simple_extractor.getPlainText() };

    // THEN
    EXPECT_EQ(expected_text, parsed_text);
}

INSTANTIATE_TEST_SUITE_P(
    StandardFilterTests, CallbackTest,
    ::testing::Values(
        std::make_tuple("1.pst", "1.pst.2.out", doctotext::StandardFilter::filterByMailMinCreationTime(1644216799))
                      ));

class HTMLWriterTest : public ::testing::TestWithParam<const char*>
{
protected:
    static constexpr std::string_view test_directory{ "../../tests/" };
};

TEST_P(HTMLWriterTest, SimpleExtractorTest)
{
    // GIVEN
    auto name = GetParam();
    std::string file_name{ std::string{ test_directory.data() } + name };

    std::ifstream ifs{ file_name + ".out.html" };
    ASSERT_TRUE(ifs.good()) <<  "File " << file_name << ".out.html" << " not found\n";
    
    std::string expected_text{ std::istreambuf_iterator<char>{ifs},
        std::istreambuf_iterator<char>{}};

    SCOPED_TRACE("file_name = " + file_name);

    // WHEN
    doctotext::SimpleExtractor simple_extractor{ file_name, "../plugins" }; // create a simple extractor
    std::string parsed_text{ simple_extractor.getHtmlText() };
        
    // THEN
    EXPECT_EQ(expected_text, parsed_text);
}

INSTANTIATE_TEST_SUITE_P(
    SimpleExtractorHTMLTest, HTMLWriterTest,
    ::testing::Values(
        "1.docx", "2.docx", "3.docx", "4.docx", "5.docx", "6.docx", "7.docx", "8.docx", "9.docx", "10.docx",
        "1.doc", "2.doc", "3.doc", "4.doc", "5.doc", "6.doc", "7.doc", "8.doc", "9.doc",
        "1.html", "2.html", "3.html", "4.html", "5.html", "6.html", "7.html", "8.html", "9.html"
                      ),
    [](const ::testing::TestParamInfo<HTMLWriterTest::ParamType>& info) {
        std::string file_name = info.param;
        dots_to_underscores(file_name);

        std::string name = file_name + "_basic_html_tests";
        return name;
    });

class MiscDocumentTest : public ::testing::TestWithParam<const char*>
{
protected:
    static constexpr std::string_view test_directory{ "../../tests/" };
};

TEST_P(MiscDocumentTest, SimpleExtractorTest)
{
    // GIVEN
    auto name = GetParam();
    std::string file_name{ std::string{ test_directory.data() } + name };

    std::ifstream ifs{ file_name + ".out" };
    ASSERT_TRUE(ifs.good()) <<  "File " << file_name << ".out" << " not found\n";
    
    std::string expected_text{ std::istreambuf_iterator<char>{ifs},
        std::istreambuf_iterator<char>{}};

    SCOPED_TRACE("file_name = " + file_name);

    // WHEN
    doctotext::SimpleExtractor simple_extractor{ file_name, "../plugins" }; // create a simple extractor
    simple_extractor.addParameters({"TESSDATA_PREFIX", std::string("../tessdata")});
    std::string parsed_text{ simple_extractor.getPlainText() };
        
    // THEN
    EXPECT_EQ(expected_text, parsed_text);
}

INSTANTIATE_TEST_SUITE_P(
    MiscellaneousTest, MiscDocumentTest,
    ::testing::Values(
        "biff5.xls",
        "hebrew_1.rtf",
        "hebrew_2.rtf",
        "japanese.xls",
        "embedded_spreadsheet.doc",
        "encoding_in_table.doc",
        "fields.doc",
        "fields.docx",
        "crash_test.xls",
        "special_para_cmds.rtf",
        "header_footer.doc",
        "shared_strings.xls",
        "shared_strings.xlsb",
        "whitespaces.odt",
        "whitespaces.doc",
        "whitespaces.docx",
        "ole.odt",
        "ole.fodt",
        "nested_tables.html",
        "first.eml",
        "second.eml",
        "third.eml",
        "fourth.eml",
        "html_with_doc_ext.doc",
        "html_with_xls_ext.xls",
        "rtf_with_doc_ext.doc",
        "comments_libreoffice_3.5.odt",
        "comments_libreoffice_3.5.doc",
        "comments_libreoffice_3.5.docx",
        "comments_libreoffice_3.5.rtf",
        "gb18030_without_bom.text",
        "utf16_with_bom.text",
        "test.asp",
        "test.aspx",
        "test.css",
        "test.fpp",
        "test.js",
        "test.json",
        "test.txt",
        "test.xml",
        "empty_cells.xlsx",
        "test.zip",
        "test.tar",
        "test.tar.gz",
        "test.tar.bz2",
        "test.tar.xz",
        "test.rar"
                      ),
    [](const ::testing::TestParamInfo<MiscDocumentTest::ParamType>& info) {
        std::string file_name = info.param;
        dots_to_underscores(file_name);

        std::string name = file_name + "_simple_extractor_test";
        return name;
    });

class PasswordProtectedTest : public ::testing::TestWithParam<const char*>
{
protected:
    static constexpr std::string_view test_directory{ "../../tests/" };
};

TEST_P(PasswordProtectedTest, MajorTestingModule)
{
    // GIVEN
    auto format = GetParam();
    std::string file_name{ std::string{ test_directory.data() } + "password_protected." + format };

    std::ifstream ifs{ file_name + ".out" };
    ASSERT_TRUE(ifs.good()) <<  "File " << file_name << ".out" << " not found\n";
    
    std::string expected_text{ std::istreambuf_iterator<char>{ifs},
        std::istreambuf_iterator<char>{}};

    SCOPED_TRACE("file_name = " + file_name);

    // WHEN
    doctotext::SimpleExtractor simple_extractor{ file_name, "../plugins" }; // create a simple extractor
        
    try 
    {
        std::string parsed_text{ simple_extractor.getPlainText() };
        FAIL() << "We are not supporting password protected files yet. Why didn\'t we catch exception?\n";
    }
    catch (doctotext::Exception& ex)
    {
        std::string test_text {
            "Error processing file " + file_name + ".\n" + ex.getBacktrace()
        };
        
        EXPECT_EQ(test_text, expected_text);
    }   
}

INSTANTIATE_TEST_SUITE_P(
    SimpleExtractorTests, PasswordProtectedTest,
    ::testing::Values(
        "doc", "docx", "key", "pages", "numbers", "odp", "pdf", "ppt", "pptx", "xls", "xlsb", "xlsx"
                      ),
    [](const ::testing::TestParamInfo<PasswordProtectedTest::ParamType>& info) {
        std::string format = info.param;
        std::string name = format + "_password_protected_test";
        return name;
    });

class MultithreadedTest : public ::testing::TestWithParam<std::tuple<int, int, const char*>>
{
protected:
    static constexpr std::string_view test_directory{ "../../tests/" };
};

void* thread_func(void* data)
{
	std::string* file_name = (std::string*)data;

	doctotext::SimpleExtractor extractor{ *file_name, "../plugins" };
    try {
      extractor.getPlainText();
      extractor.getMetaData();
    } catch (doctotext::Exception& ex) {
        return new bool(false);
    }

	pthread_exit(NULL);
}

TEST_P(MultithreadedTest, SimpleExtractorTests)
{
    const auto [lower, upper, format] = GetParam();

    std::vector<pthread_t> threads;

    std::vector<std::string> file_names(upper - lower + 1);

    std::generate(file_names.begin(), file_names.end(), [format_str = format, i = lower]() mutable
    {
        return test_directory.data() + std::to_string(i++) + "." + format_str;
    });

    for(auto& file_name : file_names)
    {
        // GIVEN
      SCOPED_TRACE("file_name = " + file_name);

        // WHEN
      pthread_t thread;
      int res = pthread_create(&thread, NULL, thread_func, (void*)&file_name);

      EXPECT_FALSE(res) << "Error creating thread " << res;
      threads.push_back(thread);
    }

    // THEN
    bool all_ok = true;
    for (int i = 0; i < threads.size(); i++)
    {
      void *status;
      int res = pthread_join(threads[i], &status);
      if (!res)
        std::cerr << "Thread " << i << " finished successfully." << std::endl;
      else
      {
        std::cerr << "Thread " << i << " finished with error." << std::endl;
        all_ok = false;
      }
    }
    ASSERT_TRUE(all_ok);
}


INSTANTIATE_TEST_SUITE_P(
    BasicTests, MultithreadedTest,
    ::testing::Values(
        std::make_tuple(1, 9, "odt"),
        std::make_tuple(1, 9, "fodt"),
        std::make_tuple(1, 9, "ods"),
        std::make_tuple(1, 9, "fods"),
        std::make_tuple(1, 9, "odp"),
        std::make_tuple(1, 9, "fodp"),
        std::make_tuple(1, 9, "odg"),
        std::make_tuple(1, 9, "fodg"),
        std::make_tuple(1, 9, "rtf"),
        std::make_tuple(1, 9, "doc"),
        std::make_tuple(1, 9, "xls"),
        std::make_tuple(1, 9, "xlsb"),
        std::make_tuple(1, 9, "pdf"),
        std::make_tuple(1, 9, "ppt"),
        std::make_tuple(1, 9, "docx"),
        std::make_tuple(1, 9, "xlsx"),
        std::make_tuple(1, 9, "pptx"),
        std::make_tuple(1, 9, "pages"),
        std::make_tuple(1, 9, "numbers"),
        std::make_tuple(1, 9, "key"),
        std::make_tuple(1, 9, "html"),
        std::make_tuple(1, 1, "pst")
                      ),
    [](const ::testing::TestParamInfo<MultithreadedTest::ParamType>& info) {
      std::string name = std::string{ std::get<2>(info.param) } + "_multithreaded_tests";
      return name;
    });

class MultiPageFilterTest : public ::testing::TestWithParam<std::tuple<int, int, const char*>>
{
protected:
  static constexpr std::string_view test_directory{ "../../tests/" };
};

TEST_P(MultiPageFilterTest, SimpleExtractorTests)
{
  const auto [lower, upper, format] = GetParam();
  const int MAX_PAGES = 2;
  std::string prefix = "multi_pages_";
  std::vector<std::string> file_names(upper - lower + 1);
  std::generate(file_names.begin(), file_names.end(), [format_str = format, prefix, i = lower]() mutable
  {
    return test_directory.data() + prefix + std::to_string(i++) + "." + format_str;
  });

  for(auto& file_name : file_names)
  {
    std::ifstream ifs{ file_name + ".out" };
    ASSERT_TRUE(ifs.good()) <<  "File " << file_name << ".out" << " not found\n";

    std::string expected_text{ std::istreambuf_iterator<char>{ifs},
                               std::istreambuf_iterator<char>{}};

    SCOPED_TRACE("file_name = " + file_name);

    // WHEN
    doctotext::SimpleExtractor simple_extractor{ file_name, "../plugins" }; // create a simple extractor
    simple_extractor.addChainElement(new doctotext::TransformerFunc([MAX_PAGES, counter = 0](doctotext::Info &info) mutable
                                   {
                                     if (info.tag_name == doctotext::StandardTag::TAG_PAGE) {++counter;}
                                     if (info.tag_name == doctotext::StandardTag::TAG_PAGE && counter > MAX_PAGES) {info.cancel = true;}
                                   }));
    std::string parsed_text{ simple_extractor.getPlainText() };

    // THEN
    EXPECT_EQ(expected_text, parsed_text);
  }

}

namespace
{
	std::string read_test_file(const std::string& file_name)
	{
		std::ifstream stream;
		stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		stream.open("../../tests/" + file_name);
		return std::string{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
	}
}

INSTANTIATE_TEST_SUITE_P(
        BasicTests, MultiPageFilterTest,
        ::testing::Values(
                std::make_tuple(1, 1, "pdf")
        ),
        [](const ::testing::TestParamInfo<MultithreadedTest::ParamType>& info) {
          std::string name = std::string{ std::get<2>(info.param) } + "_multi_page_filter_tests";
          return name;
        });

TEST(HtmlWriter, RestoreAttributes)
{
  using namespace doctotext;
	std::shared_ptr<doctotext::ParserManager> parser_manager(new doctotext::ParserManager{ "../plugins" });
	std::stringstream output;
	std::ifstream in("../../tests/1.html");
	Input(&in)
		| Importer(doctotext::ParserParameters(), parser_manager)
		| HtmlExporter(output, HtmlExporter::RestoreOriginalAttributes{true});

	EXPECT_EQ(read_test_file("1.html.restore_attributes.out.html"), output.str());
}
