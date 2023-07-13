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

#include "log.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <pthread.h>
#include <sstream>

namespace doctotext
{

std::ostream& operator<<(std::ostream& stream, severity_level severity)
{
	switch (severity)
	{
		case debug: stream << std::string("DEBUG"); break;
		case info: stream << std::string("INFO"); break;
		case warning: stream << std::string("WARNING"); break;
		case error: stream << std::string("ERROR"); break;
	}
	return stream;
}

severity_level log_verbosity = info;

std::ostream* log_stream = &std::clog;

class default_log_record_stream : public std::ostringstream
{
public:
	default_log_record_stream(severity_level severity, source_location location)
		: m_severity(severity), m_location(location)
	{
	}

	~default_log_record_stream()
	{
		std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		*log_stream << std::put_time(std::localtime(&t), "%FT%T%z") <<
			" " << m_severity <<
			" " << std::filesystem::path(m_location.file_name).filename() <<
			":" << m_location.line <<
			" " << m_location.function_name <<
			" " << str() << std::endl;
	}

private:
	severity_level m_severity;
	source_location m_location;
};

std::function<std::unique_ptr<std::ostream>(severity_level severity, source_location location)> create_log_record_stream =
[](severity_level severity, source_location location) -> std::unique_ptr<std::ostream>
{
	return std::make_unique<default_log_record_stream>(severity, location);
};

static pthread_mutex_t cerr_log_redirection_mutex = PTHREAD_MUTEX_INITIALIZER;

cerr_log_redirection::cerr_log_redirection(source_location location)
	: m_redirected(false), m_cerr_buf_backup(nullptr), m_location(location)
{
	redirect();
}

cerr_log_redirection::~cerr_log_redirection()
{
	if (m_redirected)
		restore();
}

void cerr_log_redirection::redirect()
{
	if (log_verbosity_includes(debug))
	{
		m_log_record_stream = create_log_record_stream(debug, m_location);
		pthread_mutex_lock(&cerr_log_redirection_mutex);
		m_cerr_buf_backup = std::cerr.rdbuf(m_log_record_stream->rdbuf());
	}
	else
		std::cerr.setstate(std::ios::failbit);
	m_redirected = true;
}

void cerr_log_redirection::restore()
{
	if (m_cerr_buf_backup != nullptr)
	{
		std::cerr.rdbuf(m_cerr_buf_backup);
		pthread_mutex_unlock(&cerr_log_redirection_mutex);
		m_cerr_buf_backup = nullptr;
	}
	else
		std::cerr.clear();
	m_redirected = false;
}

} // namespace doctotext
