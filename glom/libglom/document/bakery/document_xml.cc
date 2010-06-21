/*
 * Copyright 2000-2002 Murray Cumming
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <libglom/document/bakery/document_xml.h>


namespace GlomBakery
{


Document_XML::Document_XML()
: m_pDOM_Document(0),
  m_write_formatted(false)
{
}

Document_XML::~Document_XML()
{
  //m_pDOM_Document is owned by m_DOM_Document;
}

bool Document_XML::load_after(int& failure_code)
{
  //Initialize the output parameter:
  failure_code = 0;

  const bool bTest = type_base::load_after(failure_code);
  if(!bTest)
    return false; //Failed.

#ifdef LIBXMLCPP_EXCEPTIONS_ENABLED
  try
#endif
  {
    //Link the parser to the XML text that was loaded:
    //m_DOM_Parser.setDoValidation(true);

    if(m_strContents.empty())
      g_warning("Document_XML::load_after(): parsing empty document.");

    m_DOM_Parser.parse_memory(m_strContents);
    m_pDOM_Document = m_DOM_Parser.get_document();
    if(!m_pDOM_Document) return false;

    return true; //Success.
  }
#ifdef LIBXMLCPP_EXCEPTIONS_ENABLED
  catch(const std::exception& ex)
  {
    std::cerr << "XML Parser error: \n" << ex.what() << std::endl;

    return false; //Failed.
  }
#endif
}


bool Document_XML::save_before()
{
  if(get_modified())
  {
    //Write XML to string:
    m_strContents.erase();

    Util_DOM_Write(m_strContents);

    //Save the XML string:
    return type_base::save_before();
  }
  else
  {
    return true; //Success. (At doing nothing, because nothing needed to be done.)
  }
}

//TODO: Why is this parameter here if it is not used?
bool Document_XML::set_xml(const Glib::ustring& /* strXML */)
{ 
#ifdef LIBXMLCPP_EXCEPTIONS_ENABLED
  try
#endif
  {
    if(m_strContents.empty())
      g_warning("Document_XML::set_xml(): parsing empty document.");

    m_DOM_Parser.parse_memory(m_strContents);
    m_pDOM_Document = m_DOM_Parser.get_document();
    if(!m_pDOM_Document)
      return false;

    return true;	
  }
#ifdef LIBXMLCPP_EXCEPTIONS_ENABLED
  catch(const std::exception& ex)
  {
    std::cerr << "XML Parser error: \n" << ex.what() << std::endl;
    return false; //Failed.
  }
#endif
} 


Glib::ustring Document_XML::get_xml() const
{
  //Write XML to string:
  Glib::ustring strXML;
  Util_DOM_Write(strXML);
  return strXML;
}

void Document_XML::Util_DOM_Write(Glib::ustring& refstrXML) const
{
#ifdef LIBXMLCPP_EXCEPTIONS_ENABLED
  try
#endif
  {
    if(m_write_formatted)
      refstrXML = m_pDOM_Document->write_to_string_formatted();
    else
      refstrXML = m_pDOM_Document->write_to_string();
  }
#ifdef LIBXMLCPP_EXCEPTIONS_ENABLED
  catch(xmlpp::exception& ex)
  {
    std::cerr << G_STRFUNC << ": exception caught: " << ex.what() << std::endl;
  }
#endif
}

Glib::ustring Document_XML::get_node_attribute_value(const xmlpp::Element* node, const Glib::ustring& strAttributeName)
{
  if(node)
  {
    const xmlpp::Attribute* attribute = node->get_attribute(strAttributeName);
    if(attribute)
    {
      Glib::ustring value = attribute->get_value(); //Success.
      return value;
    }
  }

  return ""; //Failed.
}

void Document_XML::set_node_attribute_value(xmlpp::Element* node, const Glib::ustring& strAttributeName, const Glib::ustring& strValue)
{
  if(node)
  {
    xmlpp::Attribute* attribute = node->get_attribute(strAttributeName);
    if(attribute)
      attribute->set_value(strValue);
    else
    {
      if(!strValue.empty()) //Don't add an attribute if the value is empty, to keep the document smaller.
        node->set_attribute(strAttributeName, strValue);
    }
  }
}

xmlpp::Element* Document_XML::get_node_child_named(const xmlpp::Element* node, const Glib::ustring& strName)
{
  xmlpp::Element* nodeResult = 0;

  if(node)
  { 
    xmlpp::Node::NodeList list = node->get_children(strName);

    //We check all of them, instead of just the first, until we find one,
    //because get_children() returns, for instance, TextNodes (which are not Elements) for "text", 
    //as well as Elements with the name "text".
    for(xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter)
    {
      nodeResult = dynamic_cast<xmlpp::Element*>(*iter);  
      if(nodeResult)
        return nodeResult;
    }                       
  }

  return nodeResult;
}

xmlpp::Element* Document_XML::get_node_child_named_with_add(xmlpp::Element* node, const Glib::ustring& strName)
{
  xmlpp::Element* nodeResult = get_node_child_named(node, strName);

  if(!nodeResult)
    nodeResult = node->add_child(strName);

  return nodeResult;
}

void Document_XML::set_dtd_name(const std::string& strVal)
{
  m_strDTD_Name = strVal;
}

std::string Document_XML::get_dtd_name() const
{
  return m_strDTD_Name;
}

void Document_XML::set_dtd_root_node_name(const Glib::ustring& strVal, const Glib::ustring& xmlns)
{
  m_strRootNodeName = strVal;
  m_root_xmlns = xmlns;
}

Glib::ustring Document_XML::get_dtd_root_node_name() const
{
  return m_strRootNodeName;
}


const xmlpp::Element* Document_XML::get_node_document() const
{
  //Call the non-const overload:
  return const_cast<Document_XML*>(this)->get_node_document();
}

xmlpp::Element* Document_XML::get_node_document()
{
  if(!m_pDOM_Document)
    m_pDOM_Document = m_DOM_Parser.get_document();
  
  //Make sure that it has the DTD declaration:
  //TODO: Put this in a better place, where it's more guaranteed to always be set?
  //TODO: Add API to specify the PUBLIC URI, if the document should write this:
  //SYSTEM (local) DTDs do not seem very useful.
  //- means non-registered, which is commonly used.
  //m_pDOM_Document->set_internal_subset(m_strRootNodeName, "-//glom/" + m_strDTD_Name, m_strDTD_Name);

  xmlpp::Element* nodeRoot = m_pDOM_Document->get_root_node();
  if(!nodeRoot)
  {
    //Add it if it isn't there already:
    nodeRoot = m_pDOM_Document->create_root_node(m_strRootNodeName, m_root_xmlns);
  }
  
  //Make sure that it has the root name name and xmlns:
  nodeRoot->set_namespace_declaration(m_root_xmlns);

  return nodeRoot;
}

void Document_XML::set_write_formatted(bool formatted)
{
  m_write_formatted = formatted;
}


void Document_XML::add_indenting_white_space_to_node(xmlpp::Node* node, const Glib::ustring& start_indent)
{
  if(!node)
    node = get_node_document();

  //Remove any previous indenting:
  {
  xmlpp::Node::NodeList list = node->get_children();
  for(xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter)
  {
    xmlpp::Node* child = *iter;
    if(!child)
      continue;

    xmlpp::ContentNode* text = dynamic_cast<xmlpp::ContentNode*>(child);
    if(text)
    {
      if(text->is_white_space())
        node->remove_child(text);
    }
  }
  }

  //All indents have a newline, 
  //and we add spaces each time we recurse:
  Glib::ustring indent = start_indent;
  if(indent.empty())
    indent = "\n  ";
  else
    indent += "  ";

  //Add indenting text items:
  bool had_children = false;
  xmlpp::Element* node_as_element = dynamic_cast<xmlpp::Element*>(node);
  xmlpp::Node::NodeList list = node_as_element->get_children();
  for(xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter)
  {
    xmlpp::Node* child = *iter;
    if(!child)
      continue;

    xmlpp::ContentNode* text = dynamic_cast<xmlpp::ContentNode*>(child);
    if(text)
    {
      if(!text->is_white_space())
        continue; //Don't change content items.
    }

    //Add a text item for the indenting, before the child:
    //std::cout << "  Adding indent after node=" << child->get_name() << ": START" << indent << "END" << std::endl;
    node_as_element->add_child_text_before(child, indent);
    had_children = true;

    //Recurse into the children:
    add_indenting_white_space_to_node(child, indent);
  }

  //If it has children then add an indent before the closing tag:
  if(had_children)
    node_as_element->add_child_text(start_indent);
}


} //namespace GlomBakery.
