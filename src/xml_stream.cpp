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

#include "xml_stream.h"

#include "exception.h"
#include <iostream>
#include <libxml/xmlreader.h>
#include "log.h"
#include "pthread.h"

namespace doctotext
{

static size_t xml_parser_usage_counter = 0;
static pthread_mutex_t xml_parser_usage_counter_mutex = PTHREAD_MUTEX_INITIALIZER;

static void checkXmlMemGet()
{
	//in windows 64 bit there is a problem with initialization of some pointers to functions.
	//to make sure xmlMemGet will not be called twice I'm protecting this code with mutexes.
	pthread_mutex_lock(&xml_parser_usage_counter_mutex);
	if (!xmlFree)
		xmlMemGet(&xmlFree, &xmlMalloc, &xmlRealloc, NULL);
	pthread_mutex_unlock(&xml_parser_usage_counter_mutex);
}

static void initXmlParser()
{
	pthread_mutex_lock(&xml_parser_usage_counter_mutex);
	if (xml_parser_usage_counter == 0)
		xmlInitParser();
	xml_parser_usage_counter++;
	pthread_mutex_unlock(&xml_parser_usage_counter_mutex);
}

static void cleanupXmlParser()
{
	pthread_mutex_lock(&xml_parser_usage_counter_mutex);
	xml_parser_usage_counter--;
	if (xml_parser_usage_counter == 0)
		xmlCleanupParser();
	pthread_mutex_unlock(&xml_parser_usage_counter_mutex);
}

// warning TODO: Maybe it will be good direction if XmlStream will not require loading whole xml data at once? For now\
	if we want to parse one of OOXML/ODF/ODFXML files we need to load whole file into memory at once. It would be good\
	if XmlStream was more frugal

struct XmlStream::Implementation
{
	bool m_badbit;
	xmlTextReaderPtr m_reader;
	int m_curr_depth;
	bool m_manage_xml_parser;
};

XmlStream::XmlStream(const std::string &xml, bool manage_xml_parser, int xml_parse_options)
{
	impl = NULL;
	try
	{
		impl = new Implementation;
		impl->m_reader = NULL;
		impl->m_badbit = false;
		impl->m_manage_xml_parser = manage_xml_parser;
		checkXmlMemGet();
		if (impl->m_manage_xml_parser)
			initXmlParser();
		impl->m_reader = xmlReaderForMemory(xml.c_str(), xml.length(), NULL, NULL, xml_parse_options);
		if (impl->m_reader == NULL)
		{
			if (impl->m_manage_xml_parser)
				cleanupXmlParser();
			delete impl;
			impl = NULL;
			throw doctotext::Exception("Cannot initialize XmlStream: xmlReaderForMemory has failed");
		}
		if (xmlTextReaderRead(impl->m_reader) != 1)
		{
			if (impl->m_manage_xml_parser)
				cleanupXmlParser();
			xmlFreeTextReader(impl->m_reader);
			delete impl;
			impl = NULL;
			throw doctotext::Exception("Cannot initialize XmlStream: xmlTextReaderRead has failed");
		}
		doctotext_log(debug) << "# read. type=" << xmlTextReaderNodeType(impl->m_reader) << ", depth=" << xmlTextReaderDepth(impl->m_reader) << ", name=" << (char*)xmlTextReaderConstLocalName(impl->m_reader);
		impl->m_curr_depth = xmlTextReaderDepth(impl->m_reader);
		if (impl->m_curr_depth == -1)
		{
			if (impl->m_manage_xml_parser)
				cleanupXmlParser();
			xmlFreeTextReader(impl->m_reader);
			delete impl;
			impl = NULL;
			throw doctotext::Exception("Cannot initialize XmlStream: xmlTextReaderDepth has failed");
		}
		doctotext_log(debug) << "Starting curr_depth: " << impl->m_curr_depth;
	}
	catch (std::bad_alloc& ba)
	{
		if (impl)
		{
			if (impl->m_reader != NULL)
				xmlFreeTextReader(impl->m_reader);
			delete impl;
		}
		throw;
	}
}

XmlStream::~XmlStream()
{
	if (impl->m_reader != NULL)
		xmlFreeTextReader(impl->m_reader);
	if (impl->m_manage_xml_parser)
		cleanupXmlParser();
	delete impl;
}

XmlStream::operator bool()
{
	return !impl->m_badbit;
}

void XmlStream::next()
{
	doctotext_log(debug) << "# next(). curr_depth=" << impl->m_curr_depth;
	do
	{
		if (xmlTextReaderRead(impl->m_reader) != 1)
		{
			doctotext_log(debug) << "# End of file or error - Null";
			impl->m_badbit = true;
			return;
		}
		doctotext_log(debug) << "# read. type=" << xmlTextReaderNodeType(impl->m_reader) << ", depth=" << xmlTextReaderDepth(impl->m_reader) << ", name=" << (char*)xmlTextReaderConstLocalName(impl->m_reader);
		if (xmlTextReaderDepth(impl->m_reader) < impl->m_curr_depth)
		{
			impl->m_badbit = true;
			doctotext_log(debug) << "# End of level or error - Null";
			return;
		}
	} while (xmlTextReaderNodeType(impl->m_reader) == 15 || xmlTextReaderDepth(impl->m_reader) > impl->m_curr_depth);
	doctotext_log(debug) << (
		xmlTextReaderConstValue(impl->m_reader) == NULL ?
			std::string("# null value.") :
			std::string("# value:") + (char*)xmlTextReaderConstValue(impl->m_reader)
		);
	impl->m_badbit = false;
}

void XmlStream::levelDown()
{
	impl->m_curr_depth++;
	doctotext_log(debug) << "# levelDown(). curr_depth=" << impl->m_curr_depth;
	// warning TODO: <a></a> is not empty according to xmlTextReaderIsEmptyElement(). Check if it is a problem.
	if (xmlTextReaderIsEmptyElement(impl->m_reader) != 0)
	{
		impl->m_badbit = true;
		doctotext_log(debug) << "# Empty or error - Null";
		return;
	}
	do
	{
		if (xmlTextReaderRead(impl->m_reader) != 1)
		{
			impl->m_badbit = true;
			doctotext_log(debug) << "# End of document - Null";
			return;
		}
		doctotext_log(debug) << "# read. type=" << xmlTextReaderNodeType(impl->m_reader) << ", depth=" << xmlTextReaderDepth(impl->m_reader) << ", name=" << (char*)xmlTextReaderConstLocalName(impl->m_reader);
		if (xmlTextReaderDepth(impl->m_reader) < impl->m_curr_depth)
		{
			impl->m_badbit = true;
			doctotext_log(debug) << "# Level empty or error - Null";
			return;
		}
	} while (xmlTextReaderNodeType(impl->m_reader) == 15);
	doctotext_log(debug) << "# name:" << (char*)xmlTextReaderConstLocalName(impl->m_reader) << "\n";
	doctotext_log(debug) << (
		xmlTextReaderConstValue(impl->m_reader) == NULL ?
			std::string("# null value.") :
			std::string("# value:") + (char*)xmlTextReaderConstValue(impl->m_reader)
		);
}

void XmlStream::levelUp()
{
	impl->m_curr_depth--;
	doctotext_log(debug) << "# levelDown(). curr_depth=" << impl->m_curr_depth;
	if (impl->m_badbit)
	{
		doctotext_log(debug) << "# Was null - now invalid.";
		return;
	}
	for(;;)
	{
		if (xmlTextReaderRead(impl->m_reader) != 1)
		{
			impl->m_badbit = true;
			doctotext_log(debug) << "# End of document or error - Null";
			return;
		}
		doctotext_log(debug) << "# read. type=" << xmlTextReaderNodeType(impl->m_reader) << ", depth=" << xmlTextReaderDepth(impl->m_reader) << ", name=" << (char*)xmlTextReaderConstLocalName(impl->m_reader);
		if (xmlTextReaderNodeType(impl->m_reader) == 15 && xmlTextReaderDepth(impl->m_reader) == impl->m_curr_depth)
		{
			impl->m_badbit = false;
			break;
		}
	}
	doctotext_log(debug) << "# name:" << (char*)xmlTextReaderConstLocalName(impl->m_reader);
	doctotext_log(debug) << (
		xmlTextReaderConstValue(impl->m_reader) == NULL ?
			std::string("# null value.") :
			std::string("# value:") + (char*)xmlTextReaderConstValue(impl->m_reader)
		);
}

char* XmlStream::content()
{
	doctotext_log(debug) << "# content()";
	return (char*)xmlTextReaderConstValue(impl->m_reader);
}

std::string XmlStream::name()
{
	doctotext_log(debug) << "# name()";
	return (char*)xmlTextReaderConstLocalName(impl->m_reader);
}

std::string XmlStream::fullName()
{
	doctotext_log(debug) << "# fullName()";
	return (char*)xmlTextReaderConstName(impl->m_reader);
}

std::string XmlStream::stringValue()
{
	doctotext_log(debug) << "# stringValue()";
	if (xmlTextReaderNodeType(impl->m_reader) != 1)
	{
		doctotext_log(debug) << "!!! Getting string value not from start tag.";
		return "";
	}
	xmlNodePtr node = xmlTextReaderExpand(impl->m_reader);
	if (node == NULL)
		return "";
	xmlChar* val = xmlNodeListGetString(node->doc, node->xmlChildrenNode, 1);
	if (val == NULL)
		return "";
	std::string s((char*)val);
	xmlFree(val);
	return s;
}

std::string XmlStream::attribute(const std::string& attr_name)
{
	doctotext_log(debug) << "# attribute()";
	if (xmlTextReaderNodeType(impl->m_reader) != 1)
	{
		doctotext_log(debug) << "!!! Getting attribute not from start tag.";
		return "";
	}
	xmlNodePtr node = xmlTextReaderExpand(impl->m_reader);
	if (node == NULL)
		return "";
	xmlChar* attr = xmlGetProp(node, (xmlChar*)attr_name.c_str());
	if (attr == NULL)
		return "";
	std::string s((char*)attr);
	xmlFree(attr);
	return s;
}

}; // namespace doctotext
