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

#ifndef DOCTOTEXT_MISC_H
#define DOCTOTEXT_MISC_H

#include "formatting_style.h"
#include <filesystem>
#include <string>
#include <vector>
#include <wv2/ustring.h>
#include <time.h>

namespace doctotext
{
	class Link;
}
struct tm;

using namespace doctotext;
using namespace wvWare;

typedef std::vector<std::string> svector;

DllExport std::string formatTable(std::vector<svector>& mcols, const FormattingStyle& options);
DllExport std::string formatUrl(const std::string& mlink_url, const std::string& mlink_text, const FormattingStyle& options);
DllExport std::string formatList(std::vector<std::string>& mlist, const FormattingStyle& options);
DllExport std::string formatNumberedList(std::vector<std::string>& mlist);

DllExport std::string ustring_to_string(const UString& s);

/**
	Parses date and time in %Y-%m-%dT%H:%M:%S or %Y%m%d;%H%M%S format
**/
DllExport bool string_to_date(const std::string& s, tm& date);

DllExport std::string date_to_string(const tm& date);

DllExport std::string uint_to_string(unsigned int value);

DllExport UString utf8_to_ustring(const std::string& src);

DllExport std::string unichar_to_utf8(unsigned int unichar);

/**
 * @return Path for the directory that contains the specified module. The module must have been loaded by the current process.
 */
DllExport std::filesystem::path get_self_path();

/**
	UTF16 characters take from 2 to 4 bytes length.
	Code points from 0x0000 to 0xFFFF require two bytes (so called BMP, most popular characters).
	But there are rare used characters which use codes between 0x10000 to 0x10FFFF.
	In that case utf16 character requires 4 bytes. Algorithm:

	0x10000 is subtracted from the code point, leaving a 20 bit number in the range 0..0xFFFFF.
	The top ten bits (a number in the range 0..0x3FF) are added to 0xD800 to give the first code
	unit or lead surrogate, which will be in the range 0xD800..0xDBFF (previous versions of the
	Unicode Standard referred to these as high surrogates). The low ten bits (also in the range 0..0x3FF)
	are added to 0xDC00 to give the second code unit or trail surrogate, which will be in the range
	0xDC00..0xDFFF (previous versions of the Unicode Standard referred to these as low surrogates).

	In BMP set, there are no 16-bit characters which first 6 bits have 110110 or 110111. They are reserved.

	In the following function below, we check if first two bytes require another two bytes.
**/
inline bool utf16_unichar_has_4_bytes(unsigned int ch)
{
	return (ch & 0xFC00) == 0xD800;
}

/**
	All parsers should support Link class. We need to determine where all links lie in final text.
	But it may be sometimes difficult, simply because we are not parsing text in "correct" order all the time.
	Sometimes we are parsing lists, attachments, tables etc. It is easier to mark special place in the text
	where single link lies. We have two functions to help us. If we want to "save" a space for the link in text,
	we can use insertSpecialLinkBlockIntoText function. Make sure that text is already encoded in UTF8.
	This function will insert a block of "data" into given text to mark a place for link.
**/
DllExport void insertSpecialLinkBlockIntoText(std::string& text, const Link& link);

/**
	All parsers should support Link class. We need to determine where all links lie in final text.
	But it may be sometimes difficult, simply because we are not parsing text in "correct" order all the time.
	Sometimes we are parsing lists, attachments, tables etc. It is easier to mark special place in the text
	where single link lies. We have two functions to help us. First one is insertSpecialLinkBlockIntoText.
	When we are done parsing a file, we need to obtain positions of all parsed links. In that case, we need to call
	decodeSpecialLinkBlocks. It takes extracted text and vector of links. Then for each link, function
	calculates position and removes "special" characters which we have used in order to mark links.

	There is one important requirement: links in "links" vector (2 argument) must contain records
	in correct order.
**/
DllExport void decodeSpecialLinkBlocks(std::string& text, std::vector<Link>& links);

DllExport std::string int_to_str(int i);

DllExport int str_to_int(const std::string& s);

DllExport bool is_encrypted_with_ms_offcrypto(const std::string& file_name);

DllExport bool is_encrypted_with_ms_offcrypto(const char* buffer, size_t buffer_size);

DllExport tm *thread_safe_gmtime (const time_t *timer, struct tm &time_buffer);

#endif
