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

#ifndef GLOM_DATASTRUCTURE_TRANSLATABLE_ITEM_H
#define GLOM_DATASTRUCTURE_TRANSLATABLE_ITEM_H

#include <glibmm/ustring.h>
#include <map>
#include <libglom/sharedptr.h>

namespace Glom
{

///TranslatableItem have a map of translation strings - one string for each locale.
class TranslatableItem
{
public:
  TranslatableItem();
  TranslatableItem(const TranslatableItem& src);
  virtual ~TranslatableItem();

  TranslatableItem& operator=(const TranslatableItem& src);

  bool operator==(const TranslatableItem& src) const;
  bool operator!=(const TranslatableItem& src) const;

  /** Set the  non-translated identifier name.
   */
  virtual void set_name(const Glib::ustring& name);

  /** Get the non-translated identifier name.
   */
  virtual Glib::ustring get_name() const;

  bool get_name_not_empty() const; //For performance.

  virtual Glib::ustring get_title_or_name() const;

  /** Get the title's translation for the current locale.
   */
  virtual Glib::ustring get_title() const;

  /** Get the title's original (non-translated, usually English) text.
   */
  virtual Glib::ustring get_title_original() const;


  /** Set the title's translation for the current locale.
   */
  void set_title(const Glib::ustring& title);

  /** Set the title's original (non-translated, usually English) text.
   */
  void set_title_original(const Glib::ustring& title);

  void set_title_translation(const Glib::ustring& locale, const Glib::ustring& translation);

  /** Get the title's translation for the specified @a locale, optionally
   * falling back to a locale of the same language, and then falling back to 
   * the original.
   * Calling this with the current locale is the same as calling get_title_original().
   */
  Glib::ustring get_title_translation(const Glib::ustring& locale, bool fallback = true) const;
  
  /// Clear the original title and any translations of the title.
  void clear_title_in_all_locales();

  typedef std::map<Glib::ustring, Glib::ustring> type_map_locale_to_translations;

  bool get_has_translations() const;

  enum enumTranslatableItemType
  {
     TRANSLATABLE_TYPE_INVALID,
     TRANSLATABLE_TYPE_FIELD,
     TRANSLATABLE_TYPE_RELATIONSHIP,
     TRANSLATABLE_TYPE_LAYOUT_ITEM,
     TRANSLATABLE_TYPE_CUSTOM_TITLE,
     TRANSLATABLE_TYPE_PRINT_LAYOUT,
     TRANSLATABLE_TYPE_REPORT,
     TRANSLATABLE_TYPE_TABLE,
     TRANSLATABLE_TYPE_BUTTON,
     TRANSLATABLE_TYPE_TEXTOBJECT,
     TRANSLATABLE_TYPE_IMAGEOBJECT,
     TRANSLATABLE_TYPE_CHOICEVALUE
   };

  enumTranslatableItemType get_translatable_item_type();

  //Direct access, for performance:
  const type_map_locale_to_translations& _get_translations_map() const;

  static Glib::ustring get_translatable_type_name(enumTranslatableItemType item_type);

  /** The non-translated name is used for the context in gettext .po files.
   */
  static Glib::ustring get_translatable_type_name_nontranslated(enumTranslatableItemType item_type);


  /** Set the locale used for titles, to test translations.
   * Usually the current locale is just the locale at startup.
   */
  static void set_current_locale(const Glib::ustring& locale);

  /** Get the locale used by this program when it was started.
   */
  static Glib::ustring get_current_locale();

  /** Set the locale used for original text of titles. This 
   * must usually be stored in the document. 
   * Ideally, it would be English.
   */
  static void set_original_locale(const Glib::ustring& locale);

  static bool get_current_locale_not_original();

private:

  /** Get the locale used as the source language.
   * This is the language of the title that is used when there are no translations.
   */
  static Glib::ustring get_original_locale();


protected:
  enumTranslatableItemType m_translatable_item_type;

private:
  Glib::ustring m_name; //Non-translated identifier;
  Glib::ustring m_title; //The original, untranslated (usually-English) title.
  type_map_locale_to_translations m_map_translations;

  static Glib::ustring m_current_locale, m_original_locale;
};

template <class T_object>
Glib::ustring glom_get_sharedptr_name(const sharedptr<T_object>& item)
{
  if(item)
    return item->get_name();
  else
    return Glib::ustring();
}

template <class T_object>
Glib::ustring glom_get_sharedptr_title_or_name(const sharedptr<T_object>& item)
{
  if(item)
    return item->get_title_or_name();
  else
    return Glib::ustring();
}

} //namespace Glom

#endif //GLOM_DATASTRUCTURE_TRANSLATABLE_ITEM_H

