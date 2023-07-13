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

#ifndef DOCTOTEXT_LOG_H
#define DOCTOTEXT_LOG_H

#include "defines.h"
#include <functional>
#include <memory>
#include <string>

namespace doctotext
{

enum severity_level
{
	debug,
	info,
	warning,
	error
};

DllExport std::ostream& operator<<(std::ostream& stream, severity_level severity);

struct DllExport source_location
{
	std::string file_name;
	unsigned int line;
	std::string function_name;
};

extern DllExport severity_level log_verbosity;

extern DllExport std::ostream* log_stream;

extern DllExport std::function<std::unique_ptr<std::ostream>(severity_level severity, source_location location)> create_log_record_stream;

inline void current_function_helper()
{
#if defined(__GNUC__)
#define doctotext_current_function __PRETTY_FUNCTION__
#elif defined(__FUNCSIG__)
#define doctotext_current_function __FUNCSIG__
#else
#define doctotext_current_fuction __func__
#endif
}

inline bool log_verbosity_includes(severity_level severity)
{
	return severity >= log_verbosity;
}

#define doctotext_current_source_location() \
	doctotext::source_location{__FILE__, __LINE__, doctotext_current_function}

#define doctotext_log(severity) \
	if (!doctotext::log_verbosity_includes(severity)) \
	{ \
	} \
	else \
		(*doctotext::create_log_record_stream(severity, doctotext_current_source_location()))

class DllExport cerr_log_redirection
{
public:
	cerr_log_redirection(source_location location);
	~cerr_log_redirection();
	void redirect();
	void restore();

private:
	bool m_redirected;
	std::unique_ptr<std::ostream> m_log_record_stream;
	std::streambuf* m_cerr_buf_backup;
	source_location m_location;
};

} // namespace doctotext

#endif
