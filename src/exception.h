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

#ifndef DOCTOTEXT_EXCEPTION_H
#define DOCTOTEXT_EXCEPTION_H

#include <exception>
#include <list>
#include <string>
#include "defines.h"

namespace doctotext
{
	/**
		This class is implementation of std::exception, which is used by DocToText.
		In this implementation, errors can be formed in "stack".

		In order to create exception just call:
		\code
		throw Exception("First error");
		\endcode

		You can catch exception and add one more error:
		\code
		catch (Exception& ex)
		{
			ex.appendError("Next error message");
			throw;
		}
		\endcode

		or you can catch exception and get "backtrace":
		\code
		catch (Exception& ex)
		{
			doctotext_log(error) << ex.getBacktrace();
		}
		\endcode
	**/
	class DllExport Exception : public std::exception
	{
		private:
			struct Implementation;
			Implementation* impl;

		public:

			Exception() noexcept;

			/**
				The constructor.
				\param first_error_message first error message (gives information about cause of an error).
			**/
			explicit Exception(const std::string& first_error_message) noexcept;

			Exception(const Exception& ex) noexcept;

			~Exception() noexcept;

			Exception& operator = (const Exception& ex) noexcept;

			const char* what(){ return "doctotext_exception"; }

			/**
				Returns a string with all error messages. Each error message is numbered and separated by "\n".
				Suppose we have thrown an exception:
				\code
				throw Exception("First error message");
				\endcode
				Next, we have added one more error:
				\code
				ex.appendError("Second error message");
				\endcode
				In the result getBacktrace will return a string: "Backtrace:\n1. First error message\n2. Second error message\n"
			**/
			std::string getBacktrace();

			/**
				Adds one more error message.
			**/
			void appendError(const std::string& error_message);

			/**
				returns an iterator to the first error message.
			**/
			std::list<std::string>::iterator getErrorIterator() const;

			/**
				Returns a number of error messages.
			**/
			size_t getErrorCount() const;
	};

    class EncryptedFileException : public Exception
    {
    public:
        explicit EncryptedFileException(const std::string &msg)
            : Exception(msg)
        {}
    };
} // namespace doctotext

#endif
