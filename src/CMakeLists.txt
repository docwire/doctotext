set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INSTALL_RPATH .)

set(DOCTOTEXT_SOURCES
        doctotext_c_api.cpp
        attachment.cpp
        exception.cpp
        doctotext_link.cpp
        list_style.cpp
        metadata.cpp
        misc.cpp
        variant.cpp
        language.cpp
        parser_parameters.cpp
        thread_safe_ole_storage.cpp
        thread_safe_ole_stream_reader.cpp
        data_stream.cpp
        parser.cpp
        parser_manager.cpp
        parser_parameters.cpp
        html_writer.cpp
        plain_text_writer.cpp
        standard_filter.cpp
        simple_extractor.cpp
        importer.cpp
        exporter.cpp
        transformer.cpp
        parsing_chain.cpp
        meta_data_writer.cpp)

if(MSVC)
    add_definitions(-DMSVC_BUILD)
endif()

if (DOCTOTEXT_SHARED)
   add_library(doctotext_core SHARED ${DOCTOTEXT_SOURCES})
else()
   add_library(doctotext_core STATIC ${DOCTOTEXT_SOURCES})
endif()

if(MSVC)
    configure_file(/bin/wv2.dll ${CMAKE_BINARY_DIR}/wv2.dll COPYONLY)
    find_library(wvWare wv2 HINTS "C:/lib/static")
    find_library(pthread_d pthreadVC3d HINTS "C:/vcpkg/packages/pthreads_x64-windows/debug/lib")
    find_library(pthread pthreadVC3 HINTS "C:/vcpkg/packages/pthreads_x64-windows/lib")
    target_link_directories(doctotext_core PUBLIC "C:/lib")
    target_link_libraries(doctotext_core PRIVATE general ${wvWare}
                                                 debug ${pthread_d}
                                                 optimized ${pthread}
                                                 debug boost_filesystem-vc140-mt-gd
                                                 optimized boost_filesystem-vc140-mt
                                                 debug boost_system-vc140-mt-gd
                                                 optimized boost_system-vc140-mt)

    set_target_properties( doctotext_core
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/build"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/build"
            )
    target_compile_options(doctotext_core PUBLIC "/Zc:__cplusplus")

else()
    target_link_libraries(doctotext_core PRIVATE wv2 pthread boost_filesystem boost_system)
endif()

target_compile_options(doctotext_core PUBLIC ${EXTRA_FLAGS})

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(doctotext_core PRIVATE dl)
endif()

add_library(doctotext_parsers SHARED
        entities.cpp
        doctotext_unzip.cpp
        oshared.cpp
        xml_fixer.cpp
        xml_stream.cpp
        html_parser.cpp
        eml_parser.cpp
        doc_parser.cpp
        common_xml_document_parser.cpp
        iwork_parser.cpp
        odf_ooxml_parser.cpp
        odfxml_parser.cpp
        pdf_parser.cpp
        ppt_parser.cpp
        rtf_parser.cpp
        txt_parser.cpp
        xls_parser.cpp
        xlsb_parser.cpp
        pst_parser.cpp
        basic_parser_provider.cpp)

include(FindLibXml2)
find_package(ZLIB REQUIRED)

target_link_directories(doctotext_parsers PUBLIC "C:/lib")
target_link_directories(doctotext_core PUBLIC "C:/lib")

target_include_directories(doctotext_parsers PUBLIC
        ../
        .
        ${LIBXML2_INCLUDE_DIR}
        "C:/include"
        )

target_include_directories(doctotext_core PUBLIC
        ../
        .
        "C:/include"
        )
if(MSVC)
    target_link_libraries(doctotext_parsers PRIVATE
            general podofo
            general unzip
            general ${wvWare}
            general htmlcxx
            general ${LIBXML2_LIBRARIES}
            general charsetdetect
            general libbfio
            general libpff
            general doctotext_core
            debug freetyped
            optimized freetype
            debug ${pthread_d}
            optimized ${pthread}
            debug zlibd
            optimized zlib
            debug libmimetic_d
            optimized libmimetic
            )
    target_compile_options(doctotext_parsers PUBLIC "/Zc:__cplusplus")
else()
    find_package(Freetype REQUIRED)
    find_package(LibXml2 REQUIRED)
    if(APPLE)
        find_package(JPEG REQUIRED)
        find_package(PNG REQUIRED)
        find_package(OpenSSL REQUIRED)
        find_package(TIFF REQUIRED)
        set(podofo_deps, "${JPEG_LIBRARIES} PNG::PNG OpenSSL::SSL OpenSSL::Crypto TIFF::TIFF")
    endif()
    target_link_libraries(doctotext_parsers PRIVATE
        podofo ${podofo_deps}
        freetype
        unzip
        wv2
        htmlcxx
        LibXml2::LibXml2
        pthread
        z
        charsetdetect
        mimetic
        bfio
        pff
        doctotext_core
        )
endif()

add_library(doctotext_ocr SHARED ocr_parser_provider.cpp ocr_parser.cpp)
if(MSVC)
    set_property(TARGET doctotext_ocr PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    target_link_libraries(doctotext_ocr debug leptonica-1.82.0d
                                        debug tesseract51d
                                        optimized leptonica-1.82.0
                                        optimized tesseract51
                                        general doctotext_core)
    target_include_directories(doctotext_ocr PUBLIC
            ../
            .
            "C:/include"
            )
    target_compile_options(doctotext_ocr PUBLIC "/Zc:__cplusplus")
else()
    target_link_libraries(doctotext_ocr PRIVATE lept tesseract doctotext_core)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
   target_link_libraries(doctotext_parsers PRIVATE
      iconv
   )
   set_target_properties(doctotext_parsers PROPERTIES PREFIX "")
   set_target_properties(doctotext_core PROPERTIES PREFIX "")
endif()

add_executable(c_text_extractor c_text_extractor.c)
target_link_libraries(c_text_extractor PRIVATE doctotext_core)

add_executable(text_extractor text_extractor.cpp)
target_link_libraries(text_extractor PRIVATE doctotext_core)

if (DOCTOTEXT_TRACE)
   target_sources(doctotext_core PRIVATE tracing)
   add_compile_definitions(TRACE)
   add_compile_options($<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-finstrument-functions>)
endif()

install(TARGETS c_text_extractor
   DESTINATION ${CMAKE_BINARY_DIR})
install(TARGETS text_extractor
   DESTINATION ${CMAKE_BINARY_DIR})
install(TARGETS doctotext_ocr
        DESTINATION ${CMAKE_BINARY_DIR}/plugins)
install(TARGETS doctotext_parsers
        DESTINATION ${CMAKE_BINARY_DIR}/plugins)
install(TARGETS doctotext_core
        DESTINATION ${CMAKE_BINARY_DIR})

INSTALL(FILES ${CMAKE_BINARY_DIR}/src/Debug/doctotext_ocr.pdb ${CMAKE_BINARY_DIR}/src/Debug/doctotext_parsers.pdb
        DESTINATION  ${CMAKE_BINARY_DIR}/plugins
        CONFIGURATIONS Debug
        )

configure_file(${CMAKE_SOURCE_DIR}/src/doctotext_c_api.h ${CMAKE_BINARY_DIR}/doctotext_c_api.h COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/metadata.h ${CMAKE_BINARY_DIR}/metadata.h COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/formatting_style.h ${CMAKE_BINARY_DIR}/formatting_style.h COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/doctotext_link.h ${CMAKE_BINARY_DIR}/doctotext_link.h COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/exception.h ${CMAKE_BINARY_DIR}/exception.h COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/attachment.h ${CMAKE_BINARY_DIR}/attachment.h COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/variant.h ${CMAKE_BINARY_DIR}/variant.h COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/parser.h ${CMAKE_BINARY_DIR}/parser.h COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/parser_manager.h ${CMAKE_BINARY_DIR}/parser_manager.h COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/parser_builder.h ${CMAKE_BINARY_DIR}/parser_builder.h COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/parser_parameters.h ${CMAKE_BINARY_DIR}/parser_parameters.h COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/basic_parser_provider.h ${CMAKE_BINARY_DIR}/basic_parser_provider.h COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/parser_provider.h ${CMAKE_BINARY_DIR}/parser_provider.h COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/standard_filter.h ${CMAKE_BINARY_DIR}/standard_filter.h COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/writer.h ${CMAKE_BINARY_DIR}/writer.h COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/plain_text_writer.h ${CMAKE_BINARY_DIR}/plain_text_writer.h COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/html_writer.h ${CMAKE_BINARY_DIR}/html_writer.h COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/importer.h ${CMAKE_BINARY_DIR}/importer.h COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/exporter.h ${CMAKE_BINARY_DIR}/exporter.h COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/transformer.h ${CMAKE_BINARY_DIR}/transformer.h COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/parsing_chain.h ${CMAKE_BINARY_DIR}/parsing_chain.h COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/simple_extractor.h ${CMAKE_BINARY_DIR}/simple_extractor.h COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/defines.h ${CMAKE_BINARY_DIR}/defines.h COPYONLY)


if(${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR APPLE)
    file(WRITE ${CMAKE_BINARY_DIR}/text_extractor.sh "BASEDIR=$(dirname \"$0\")
    LD_LIBRARY_PATH=$BASEDIR:$$LD_LIBRARY_PATH ./$BASEDIR/text_extractor \"$@\"")
    execute_process(COMMAND chmod u+x ${CMAKE_BINARY_DIR}/text_extractor.sh)

   file(WRITE ${CMAKE_BINARY_DIR}/c_text_extractor.sh "BASEDIR=$(dirname \"$0\")
   LD_LIBRARY_PATH=$BASEDIR:$$LD_LIBRARY_PATH ./$BASEDIR/c_text_extractor \"$@\"")
   execute_process(COMMAND chmod u+x ${CMAKE_BINARY_DIR}/c_text_extractor.sh)
endif()
