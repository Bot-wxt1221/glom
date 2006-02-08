/* Glom
 *
 * Copyright (C) 2001-2005 Murray Cumming
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "iso_codes.h"
#include <libxml++/libxml++.h>
#include "../document/document_glom.h"
#include "../utils.h"
#include <glibmm/i18n.h>
#include "config.h" //For ISO_CODES_PREFIX.

namespace IsoCodes
{

static type_list_currencies list_currencies;
static type_list_locales list_locales;

typedef std::map<Glib::ustring, Locale> type_map_locales; //ID to locale.
static type_map_locales map_locales; //For quick lookup.

type_list_currencies get_list_of_currency_symbols()
{
  if(list_currencies.empty())
  {
    const Glib::ustring filename = ISO_CODES_PREFIX "/share/xml/iso-codes/iso_4217.xml";

    try
    {
      xmlpp::DomParser parser;
      //parser.set_validate();
      parser.set_substitute_entities(); //We just want the text to be resolved/unescaped automatically.
      parser.parse_file(filename);
      if(parser)
      {
        //Walk the tree:
        const xmlpp::Node* nodeRoot = parser.get_document()->get_root_node(); //deleted by DomParser.

        xmlpp::Node::NodeList listNodes = nodeRoot->get_children("iso_4217_entry");
        for(xmlpp::Node::NodeList::const_iterator iter = listNodes.begin(); iter != listNodes.end(); iter++)
        {
          xmlpp::Element* nodeEntry = dynamic_cast<xmlpp::Element*>(*iter);
          if(nodeEntry)
          {
            Currency currency;

            const xmlpp::Attribute* attribute_code = nodeEntry->get_attribute("letter_code");
            if(attribute_code)
              currency.m_symbol = attribute_code->get_value();

            const xmlpp::Attribute* attribute_name = nodeEntry->get_attribute("currency_name");
            if(attribute_name)
            {
              Glib::ustring name = _(attribute_name->get_value().c_str());
              const char* pchTranslatedName = dgettext("iso_4217", name.c_str());
              if(pchTranslatedName)
                name = pchTranslatedName;

              currency.m_name = name;
            }

            list_currencies.push_back(currency);
          }
        }
      }
    }
    catch(const std::exception& ex)
    {
      std::cerr << "Exception while parsing iso codes (currencies): " << ex.what() << std::endl;
    }
  }

  return list_currencies;
}

Glib::ustring get_locale_name(const Glib::ustring& locale_id)
{
  //Build the list of locales, with their translated language and countries names:
  if(map_locales.empty())
  {
    //Get a list of locale IDs:
    typedef std::list<std::string> type_list_ids;
    type_list_ids list_ids;

    Glib::ustring locales_path = "/usr/share/i18n/locales/";
    try
    {
      Glib::Dir dir(locales_path);
      list_ids = type_list_ids(dir.begin(), dir.end());
    }
    catch(const Glib::FileError& ex)
    {
      std::cerr << "Glom: get_locale_name(): Could not open (or read) glibc locales directory: " << locales_path << "Error: " << ex.what() << std::endl;
    }

    //Get the (translated) language names:
    typedef std::map<Glib::ustring, Glib::ustring> type_map_language; //ID to language name.
    type_map_language map_languages;

    const Glib::ustring filename_languages = ISO_CODES_PREFIX "/share/xml/iso-codes/iso_639.xml";

    try
    {
      xmlpp::DomParser parser;
      //parser.set_validate();
      parser.set_substitute_entities(); //We just want the text to be resolved/unescaped automatically.
      parser.parse_file(filename_languages);
      if(parser)
      {
        //Walk the tree:
        const xmlpp::Node* nodeRoot = parser.get_document()->get_root_node(); //deleted by DomParser.

        xmlpp::Node::NodeList listNodes = nodeRoot->get_children("iso_639_entry");
        for(xmlpp::Node::NodeList::const_iterator iter = listNodes.begin(); iter != listNodes.end(); iter++)
        {
          xmlpp::Element* nodeEntry = dynamic_cast<xmlpp::Element*>(*iter);
          if(nodeEntry)
          {
            //TODO: There are 3 codes (not each entry has each code). Is this the correct one to identify a language?
            const xmlpp::Attribute* attribute_code = nodeEntry->get_attribute("iso_639_1_code");
            if(attribute_code)
            {
              const Glib::ustring identifier = attribute_code->get_value();
              if(!identifier.empty())
              {
                const xmlpp::Attribute* attribute_name = nodeEntry->get_attribute("name");
                if(attribute_name)
                {
                  Glib::ustring name = attribute_name->get_value();
                  const char* pchTranslatedName = dgettext("iso_639", name.c_str());
                  if(pchTranslatedName)
                    name = pchTranslatedName;

                  map_languages[identifier] = name;
                }
              }
            }
          }
        }
      }
    }
    catch(const std::exception& ex)
    {
      std::cerr << "Exception while parsing iso codes (locales): " << ex.what() << std::endl;
    }


    //Get the (translated) country names:
    typedef std::map<Glib::ustring, Glib::ustring> type_map_country; //ID to country name.
    type_map_country map_country;

    const Glib::ustring filename_countries = ISO_CODES_PREFIX "/share/xml/iso-codes/iso_3166.xml";

    try
    {
      xmlpp::DomParser parser;
      //parser.set_validate();
      parser.set_substitute_entities(); //We just want the text to be resolved/unescaped automatically.
      parser.parse_file(filename_countries);
      if(parser)
      {
        //Walk the tree:
        const xmlpp::Node* nodeRoot = parser.get_document()->get_root_node(); //deleted by DomParser.

        xmlpp::Node::NodeList listNodes = nodeRoot->get_children("iso_3166_entry");
        for(xmlpp::Node::NodeList::const_iterator iter = listNodes.begin(); iter != listNodes.end(); iter++)
        {
          xmlpp::Element* nodeEntry = dynamic_cast<xmlpp::Element*>(*iter);
          if(nodeEntry)
          {
            const xmlpp::Attribute* attribute_code = nodeEntry->get_attribute("alpha_2_code");
            if(attribute_code)
            {
              const Glib::ustring identifier = attribute_code->get_value();
              if(!identifier.empty())
              {
                const xmlpp::Attribute* attribute_name = nodeEntry->get_attribute("name");
                if(attribute_name)
                {
                  Glib::ustring name = attribute_name->get_value();
                  const char* pchTranslatedName = dgettext("iso_3166", name.c_str());
                  if(pchTranslatedName)
                    name = pchTranslatedName;

                  map_country[identifier] = name;
                }
              }
            }
          }
        }
      }
    }
    catch(const std::exception& ex)
    {
      std::cerr << "Exception while parsing iso codes (locales): " << ex.what() << std::endl;
    }

      //Use a map so we can easily check for duplicates.
    for(type_list_ids::iterator iter = list_ids.begin(); iter != list_ids.end(); ++iter)
    {
      const Glib::ustring identifier = GlomUtils::locale_simplify(*iter);

      if(map_locales.find(identifier) == map_locales.end()) //Prevent duplicates.
      {
        //Split the locale ID into language and country parts:
        Glib::ustring id_language = GlomUtils::locale_language_id(identifier);
        Glib::ustring id_country;
        if(!id_language.empty() && ((id_language.size() +1) < identifier.size()))
            id_country = identifier.substr(id_language.size() + 1);

        //Get the translated human-readable names of the language and country:
        Glib::ustring name;
        type_map_language::iterator iterFindLanguage = map_languages.find(id_language);
        if(iterFindLanguage != map_languages.end()) //Ignore languages that are not listed by iso-codes.
        {
          name += iterFindLanguage->second;

          if(!id_country.empty())
          {
            type_map_country::iterator iterFindCountry = map_country.find(id_country);
            if(iterFindCountry != map_country.end())
              name += " (" + iterFindCountry->second + ")";
            else
              name = Glib::ustring(); //Ignore locales with unnamed countries.
          }

          if(!name.empty()) //Ignore locales that are not listed by iso-codes. They are probably strange things that humans can't understand
          {
            Locale the_locale;
            the_locale.m_identifier = identifier;
            the_locale.m_name = name;
            map_locales[identifier] = the_locale;
          }
        }
      }
    }
  }

  Glib::ustring result;

  type_map_locales::const_iterator iter = map_locales.find(locale_id);
  if(iter != map_locales.end())
    result = iter->second.m_name;

  return result;
}

type_list_locales get_list_of_locales()
{
  if(list_locales.empty())
  {
    get_locale_name("temp"); //Fill the map.

    //Put the map into a list:
    for(type_map_locales::iterator iter = map_locales.begin(); iter != map_locales.end(); ++iter)
    {
      list_locales.push_back(iter->second);
    }
  }

  return list_locales;
}


} //namespace IsoCodes

