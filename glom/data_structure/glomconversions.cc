/* Glom
 *
 * Copyright (C) 2001-2004 Murray Cumming
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

#include "glomconversions.h"
#include <sstream> //For stringstream

#include <locale>     // for locale, time_put
#include <ctime>     // for struct tm
#include <iostream>   // for cout, endl


Glib::ustring GlomConversions::format_time(const tm& tm_data)
{
  return format_time( tm_data, std::locale("") /* the user's current locale */ ); //Get the current locale.
}


Glib::ustring GlomConversions::format_time(const tm& tm_data, const std::locale& locale, bool iso_format)
{
  if(iso_format)
  {
    return format_tm(tm_data, locale, 'T' /* I see no iso-format time in the list, but this looks like the standard C-locale format. murrayc*/);
  }
  else
    return format_tm(tm_data, locale, 'X' /* time */);
}

Glib::ustring GlomConversions::format_date(const tm& tm_data)
{
  return format_date( tm_data, std::locale("") /* the user's current locale */ ); //Get the current locale.
}


Glib::ustring GlomConversions::format_date(const tm& tm_data, const std::locale& locale, bool iso_format)
{
  if(iso_format)
    return format_tm(tm_data, locale, 'F' /* iso-format date */);
  else
    return format_tm(tm_data, locale, 'x' /* date */);
}


Glib::ustring GlomConversions::format_tm(const tm& tm_data, const std::locale& locale, char format)
{
  //This is based on docs found here:
  //http://www.roguewave.com/support/docs/sourcepro/stdlibref/time-put.html

  //Format it into this stream:
  typedef std::stringstream type_stream;
  type_stream the_stream;
  the_stream.imbue(locale); //Make it format things for this locale. (Actually, I don't know if this is necessary, because we mention the locale in the time_put<> constructor.

  // Get a time_put face:
  typedef std::time_put<char> type_time_put;
  typedef type_time_put::iter_type type_iterator;
  const type_time_put& tp = std::use_facet<type_time_put>(locale);

  //type_iterator begin(the_stream);
  tp.put(the_stream /* iter to beginning of stream */, the_stream, ' ' /* fill */, &tm_data, format, 0 /* 'E' */ /* use locale's alternative format */);

  Glib::ustring text = the_stream.str();

  if(locale == std::locale("") /* The user's current locale */)
  {
    //Converts from the user's current locale to utf8. I would prefer a generic conversion from any locale,
    // but there is no such function, and it's hard to do because I don't know how to get the encoding name from the std::locale()
    text = Glib::locale_to_utf8(text);
  }

  return text; //TODO: Use something like Glib::locale_to_utf8()?

  /*
  //This is based on the code in Glib::Date::format_string(), which only deals with dates, but not times:

  const std::string locale_format = Glib::locale_from_utf8("%X"); //%x means "is replaced by the locale's appropriate time representation".
  gsize bufsize = std::max<gsize>(2 * locale_format.size(), 128);

  do
  {
    const Glib::ScopedPtr<char> buf (static_cast<char*>(g_malloc(bufsize)));

    // Set the first byte to something other than '\0', to be able to
    // recognize whether strftime actually failed or just returned "".
    buf.get()[0] = '\1';
    const gsize len = strftime(buf.get(), bufsize, locale_format.c_str(), &tm_data);

    if(len != 0 || buf.get()[0] == '\0')
    {
      g_assert(len < bufsize);
      return Glib::locale_to_utf8(std::string(buf.get(), len));
    }
  }
  while((bufsize *= 2) <= 65536);

  // This error is quite unlikely (unless strftime is buggy).
  g_warning("GlomConversions::format_time(): maximum size of strftime buffer exceeded, giving up");

  return Glib::ustring();
  */
}

Glib::ustring GlomConversions::get_text_for_gda_value(Field::glom_field_type glom_type, const Gnome::Gda::Value& value)
{
  return get_text_for_gda_value(glom_type, value, std::locale("") /* the user's current locale */ ); //Get the current locale.
}

Glib::ustring GlomConversions::get_text_for_gda_value(Field::glom_field_type glom_type, const Gnome::Gda::Value& value, const std::locale& locale, bool iso_format)
{
  if(value.get_value_type() == Gnome::Gda::VALUE_TYPE_NULL) //The type can be null for any of the actual field types.
  {
    return Glib::ustring();
  }

  if(glom_type == Field::TYPE_DATE)
  {
    Gnome::Gda::Date gda_date = value.get_date();

    tm the_c_time = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    the_c_time.tm_year = gda_date.year - 1900; //C years start are the AD year - 1900. So, 01 is 1901.
    the_c_time.tm_mon = gda_date.month - 1; //C months start at 0.
    the_c_time.tm_mday = gda_date.day; //starts at 1

    return format_date(the_c_time, locale, iso_format);

    /*

    Glib::Date date((Glib::Date::Day)gda_date.day, (Glib::Date::Month)gda_date.month, (Glib::Date::Year)gda_date.year);
    return date.format_string("%x"); //%x means "is replaced by the locale's appropriate date representation".
    */
  }
  else if(glom_type == Field::TYPE_TIME)
  {
    Gnome::Gda::Time gda_time = value.get_time();

    tm the_c_time = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    the_c_time.tm_hour = gda_time.hour;
    the_c_time.tm_min = gda_time.minute;
    the_c_time.tm_sec = gda_time.second;

    return format_time(the_c_time, locale, iso_format);
  }
  else if(glom_type == Field::TYPE_NUMERIC)
  {
    const GdaNumeric* gda_numeric = value.get_numeric();
    std::string text_in_c_locale;
    if(gda_numeric && gda_numeric->number) //A char* - I assume that it formatted as per the C locale. murrayc. TODO: Do we need to look at the other fields?
      text_in_c_locale = gda_numeric->number; //What formatting does this use?

    //Get an actual numeric value, so we can get a locale-specific text representation:
    std::stringstream the_stream;
    the_stream.imbue( std::locale::classic() ); //The C locale.
    the_stream.str(text_in_c_locale); //Avoid using << because Glib::ustinrg does implicit character conversion with that.
   
    double number = 0;
    the_stream >> number;
  
    //Get the locale-specific text representation:
    std::stringstream another_stream;
    another_stream.imbue(locale); //Tell it to parse stuff as per this locale.
    another_stream << number;
    Glib::ustring text;
    text = another_stream.str();  //Avoid using << because Glib::ustinrg does implicit character conversion with that.
     
    if(locale == std::locale("") /* The user's current locale */)
    {
      //Converts from the user's current locale to utf8. I would prefer a generic conversion from any locale,
      // but there is no such function, and it's hard to do because I don't know how to get the encoding name from the std::locale()
      text = Glib::locale_to_utf8(text); 
    }
    
    return text; //Do something like Glib::locale_to_utf(), but with the specified locale instead of the current locale.
  }
  else
  {
    return value.to_string();
  }
}

Gnome::Gda::Value GlomConversions::parse_value(Field::glom_field_type glom_type, const Glib::ustring& text,  bool& success)
{
  //Put a NULL in the database for empty dates, times, and numerics, because 0 would be an actual value.
  //But we use "" for strings, because the distinction between NULL and "" would not be clear to users.
  if(text.empty())
  {
    if( (glom_type ==  Field::TYPE_DATE) || (glom_type ==  Field::TYPE_TIME) || (glom_type ==  Field::TYPE_NUMERIC) )
    {
      Gnome::Gda::Value null_value;
      return null_value;
    }
  }

  if(glom_type == Field::TYPE_DATE)
  {
    tm the_c_time = parse_date(text, success);

    Gnome::Gda::Date gda_date = {0, 0, 0};
    gda_date.year = the_c_time.tm_year + 1900; //The C time starts at 1900.
    gda_date.month = the_c_time.tm_mon + 1; //The C month starts at 0.
    gda_date.day = the_c_time.tm_mday; //THe C mday starts at 1.
  
    return Gnome::Gda::Value(gda_date);
  }
  else if(glom_type == Field::TYPE_TIME)
  {
    tm the_c_time = parse_time(text, success);

    Gnome::Gda::Time gda_time = {0, 0, 0, 0};
    gda_time.hour = the_c_time.tm_hour;
    gda_time.minute = the_c_time.tm_min;
    gda_time.second = the_c_time.tm_sec;

    return Gnome::Gda::Value(gda_time);
  }
  else if(glom_type == Field::TYPE_NUMERIC)
  {
    //Try to parse the inputted number, according to the current locale.
    std::stringstream the_stream;
    the_stream.imbue( std::locale("") /* The user's current locale */ ); //Parse it as per the current locale.
    the_stream.str(text); //Avoid << because it does implicit character conversion (though that might not be a problem here. Not sure). murrayc
    double the_number = 0;
    the_stream >> the_number;  //TODO: Does this throw any exception if the text is an invalid time?

    success = true; //Can this ever fail?
    return Gnome::Gda::Value(the_number);
  }

  success = true;
  return Gnome::Gda::Value(text);

}

tm GlomConversions::parse_date(const Glib::ustring& text, bool& success)
{
  return parse_date( text, std::locale("") /* the user's current locale */, success ); //Get the current locale.
}

tm GlomConversions::parse_date(const Glib::ustring& text, const std::locale& locale, bool& success)
{
  //return parse_tm(text, locale, 'x' /* date */);

  //This is based on docs found here:
  //http://www.roguewave.com/support/docs/sourcepro/stdlibref/time-get.html

  //Format it into this stream:
  typedef std::stringstream type_stream;


  //tm the_c_time = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  /* This seems to be necessary - when I do this, as found here ( http://www.tacc.utexas.edu/services/userguides/pgi/pgC++_lib/stdlibcr/tim_0622.htm ) then the time is correctly parsed (it is changed).
   * When not, I get just zeros.
   */
  tm the_c_time = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  std::ios_base::iostate err = std::ios_base::goodbit;  //The initialization is essential because time_get seems to a) not initialize this output argument and b) check its value.

  //For some reason, the stream must be instantiated after we get the facet. This is a worryingly strange "bug".
  type_stream the_stream;
  the_stream.imbue(locale); //Make it format things for this locale. (Actually, I don't know if this is necessary, because we mention the locale in the time_put<> constructor.
  the_stream << text;

  // Get a time_get facet:
 
  typedef std::time_get<char> type_time_get;
  typedef type_time_get::iter_type type_iterator;

  const type_time_get& tg = std::use_facet<type_time_get>(locale);

  type_iterator the_begin(the_stream);
  type_iterator the_end;

  tg.get_date(the_begin, the_end, the_stream, err, &the_c_time);

  if(err != std::ios_base::failbit)
  {
    success = true;
    return the_c_time;
  }
  else
  {
    //time_get can fail just because you have entered "1/2/1903" instead "01/02/1903", so let's try another, more liberal, way:
    Glib::Date date;
    date.set_parse(text); //I think this uses the current locale. murrayc.
    if(date.valid())
    {
      tm the_c_time = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
      the_c_time.tm_year = date.get_year() - 1900; //C years start are the AD year - 1900. So, 01 is 1901.
      the_c_time.tm_mon = date.get_month() - 1; //C months start at 0.
      the_c_time.tm_mday = date.get_day(); //starts at 1

      success = true;
      return the_c_time;
    }
    else //It really really failed.
    {
      tm blank_time = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
      success = false;
      return blank_time;
    }
  }
}


tm GlomConversions::parse_time(const Glib::ustring& text, bool& success)
{
  //return parse_time( text, std::locale("") /* the user's current locale */ ); //Get the current locale.

  //time_get() does not seem to work with non-C locales.  TODO: Try again.
  tm the_time = parse_time( text, std::locale("") /* the user's current locale */, success );
  if(success)
    return the_time;
  else
  {
    //Fallback:
    //Try interpreting it as the C locale instead.
    //For instance, time_get::get_time() does not seem to be able to parse any time in a non-C locale (even "en_US" or "en_US.UTF-8").
    return parse_time( text, std::locale::classic() /* the C locale */, success );
  }
}


tm GlomConversions::parse_time(const Glib::ustring& text, const std::locale& locale, bool& success)
{
  //The sequence of statements here seems to be very fragile. If you move things then it stops working.

  //return parse_tm(text, locale, 'X' /* time */);

  //This is based on docs found here:
  //http://www.roguewave.com/support/docs/sourcepro/stdlibref/time-get.html

  //Format it into this stream:
  typedef std::stringstream type_stream;

   //tm the_c_time = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  /* This seems to be necessary - when I do this, as found here ( http://www.tacc.utexas.edu/services/userguides/pgi/pgC++_lib/stdlibcr/tim_0622.htm ) then the time is correctly parsed (it is changed).
   * When not, I get just zeros.
   */
  tm the_c_time = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  std::ios_base::iostate err = std::ios_base::goodbit; //The initialization is essential because time_get seems to a) not initialize this output argument and b) check its value.

  type_stream the_stream;
  the_stream.imbue(locale); //Make it format things for this locale. (Actually, I don't know if this is necessary, because we mention the locale in the time_put<> constructor.

  the_stream << text;

  // Get a time_get facet:
  typedef std::istreambuf_iterator<char, std::char_traits<char> > type_iterator;
  typedef std::time_get<char, type_iterator> type_time_get;
  const type_time_get& tg = std::use_facet<type_time_get>(locale);

  type_iterator the_begin(the_stream);
  type_iterator the_end;

  tg.get_time(the_begin, the_end, the_stream, err, &the_c_time);

  if(err != std::ios_base::failbit)
  {
    success = true;
    return the_c_time;
  }
  else
  {
    tm blank_time = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    success = false;
    return blank_time;
  }
}



/* This requires an extension to the standard - time_get<>.get().:
tm GlomConversions::parse_tm(const Glib::ustring& text, const std::locale& locale, char format)
{
  //This is based on docs found here:
  //http://www.roguewave.com/support/docs/sourcepro/stdlibref/time-get.html

  //Format it into this stream:
  typedef std::stringstream type_stream;
  type_stream the_stream;
  the_stream.imbue(locale); //Make it format things for this locale. (Actually, I don't know if this is necessary, because we mention the locale in the time_put<> constructor.


  // Get a time_get facet:
  typedef std::istreambuf_iterator<char, std::char_traits<char> > type_iterator;
  typedef std::time_get<char, type_iterator> type_time_get;
  const type_time_get& tg = std::use_facet<type_time_get>(locale);

  the_stream << text;

  tm the_c_time = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  type_iterator the_begin(the_stream);
  type_iterator the_end;
  std::ios_base::iostate state;  // Unused
  std::ios_base::iostate err;
  tg.get(the_begin, the_end, the_stream, state, err, &the_c_time, format, 0);

  return the_c_time;
}
*/

  
