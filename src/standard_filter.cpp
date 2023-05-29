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

#include "standard_filter.h"
namespace doctotext
{
doctotext::NewNodeCallback
StandardFilter::filterByFolderName(const std::vector<std::string> &names)
{
  return [names](doctotext::Info &info)
  {
    if (info.tag_name != doctotext::StandardTag::TAG_FOLDER)
      return;
    auto folder_name = info.getAttributeValue<std::string>("name");
    if (folder_name)
    {
      if (!std::any_of(names.begin(), names.end(), [&folder_name](const std::string &name){return (*folder_name) == name;}))
      {
        info.skip = true;
      }
    }
  };
}

doctotext::NewNodeCallback
StandardFilter::filterByAttachmentType(const std::vector<std::string> &types)
{
  return [types](doctotext::Info &info)
  {
    if (info.tag_name != doctotext::StandardTag::TAG_ATTACHMENT)
      return;
    if (info.attributes.find("extension") == info.attributes.end())
      return;
    auto attachment_type = info.getAttributeValue<std::string>("extension");
    if (attachment_type)
    {
      if (!std::any_of(types.begin(), types.end(), [&attachment_type](const std::string &type){return (*attachment_type) == type;}))
      {
        info.skip = true;
      }
    }
  };
}

doctotext::NewNodeCallback
StandardFilter::filterByMailMinCreationTime(unsigned int min_time)
{
  return [min_time](doctotext::Info &info)
  {
    if (info.tag_name != doctotext::StandardTag::TAG_MAIL)
      return;
    auto mail_creation_time = info.getAttributeValue<unsigned int>("date");
    if (mail_creation_time)
    {
      if (*mail_creation_time < min_time)
      {
        info.skip = true;
      }
    }
  };
}

doctotext::NewNodeCallback
StandardFilter::filterByMailMaxCreationTime(unsigned int max_time)
{
  return [max_time](doctotext::Info &info)
  {
    if (info.tag_name != doctotext::StandardTag::TAG_MAIL)
      return;
    auto mail_creation_time = info.getAttributeValue<unsigned int>("date");
    if (mail_creation_time)
    {
      if (mail_creation_time > max_time)
      {
        info.skip = true;
      }
    }
  };
}

doctotext::NewNodeCallback
StandardFilter::filterByMaxNodeNumber(unsigned int max_nodes)
{
  return [max_nodes, node_no = 0](doctotext::Info &info) mutable
  {
    if (node_no++ == max_nodes)
    {
      info.cancel = true;
    }
  };
}
} // namespace doctotext
