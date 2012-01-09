/* Glom
 *
 * Copyright (C) 2001-2012 Murray Cumming
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

#include <libglom/translations_po.h>

// To read the .po files
#include <gettext-po.h>
#include "config.h" //For HAVE_GETTEXTPO_XERROR

#include <glibmm/convert.h>
#include <glibmm/fileutils.h>
#include <glibmm/datetime.h>
#include <glibmm/i18n.h>

#include <iostream>

/* For really ugly hacks! */
#include <setjmp.h>

#define GLOM_PO_HEADER \
"msgid \"\"\n" \
"msgstr \"\"\n" \
"\"Project-Id-Version: %1\\n\"\n" \
"\"product=glom&keywords=I18N+L10N&component=general\\n\"\n" \
"\"PO-Revision-Date: %2\\n\"\n" \
"\"Last-Translator: Someone <someone@someone.com>\\n\"\n" \
"\"Language-Team: %3 <someone@someone.com>\\n\"\n" \
"\"MIME-Version: 1.0\\n\"\n" \
"\"Content-Type: text/plain; charset=UTF-8\\n\"\n" \
"\"Content-Transfer-Encoding: 8bit\\n\""

namespace Glom
{

static jmp_buf jump;

static void show_gettext_error(int severity, const char* filename, const gchar* message)
{
  std::ostringstream msg_stream;
  if(filename)
    msg_stream << filename << ": ";

  if(message)
   msg_stream << message;

  switch(severity)
  {
    #ifdef PO_SEVERITY_WARNING //This was introduced in libgettext-po some time after gettext version 0.14.5 
    case PO_SEVERITY_WARNING:
    {
      // Show only debug output
      std::cout << _("Gettext-Warning: ") << msg_stream.str() << std::endl;
      break;
    }
    #endif //PO_SEVERITY_WARNING


    #ifdef PO_SEVERITY_ERROR //This was introduced in libgettext-po some time after gettext version 0.14.5 
    case PO_SEVERITY_ERROR:
    #endif //PO_SEVERITY_ERROR

    #ifdef PO_SEVERITY_FATAL_ERROR //This was introduced in libgettext-po some time after gettext version 0.14.5 
    case PO_SEVERITY_FATAL_ERROR:
    #endif //PO_SEVERITY_FATAL_ERROR

    default:
    {
      //TODO: const Glib::ustring msg = Glib::ustring(_("Gettext-Error: ")) + ' ' + msg_stream.str();
      //Gtk::MessageDialog dlg(msg, false, Gtk::MESSAGE_ERROR);
      //dlg.run();
      break;
    }
  }   
}

/*
 * The exception handling of libgettext-po is very ugly! The following methods are called
 * if an exception occurs and may not return in case of a fatal exception. We use setjmp
 * and longjmp to bypass this and return to the caller
 */
#ifdef HAVE_GETTEXTPO_XERROR
static void on_gettextpo_xerror (int severity, po_message_t /* message */, const char *filename, size_t /* lineno */, size_t /* column */,
  int /* multiline_p */, const char *message_text)
{
  show_gettext_error(severity, filename, message_text);

  #ifdef PO_SEVERITY_FATAL_ERROR  //This was introduced in libgettext-po some time after gettext version 0.14.5 
  if(severity == PO_SEVERITY_FATAL_ERROR)
    longjmp(jump, 1);
  #endif //PO_SEVERITY_FATAL_ERROR
}

static void on_gettextpo_xerror2 (int severity, po_message_t /* message1 */, const char * filename1, size_t /* lineno1 */, size_t /* column1 */,
  int /* multiline_p1 */, const char *message_text1,
  po_message_t /* message2 */, const char * /*filename2 */, size_t /* lineno2 */, size_t /* column2 */,
  int /* multiline_p2 */, const char * /* message_text2 */)
{
  show_gettext_error(severity, filename1, message_text1);
  
  #ifdef PO_SEVERITY_FATAL_ERROR  //This was introduced in libgettext-po some time after gettext version 0.14.5 
  if(severity == PO_SEVERITY_FATAL_ERROR)
    longjmp(jump, 1);
  #endif //PO_SEVERITY_FATAL_ERROR
}
#else //HAVE_GETTEXTPO_XERROR
static void on_gettextpo_error(int status, int errnum, const char * /* format */, ...)
{
  std::cerr << "gettext error (old libgettext-po API): status=" << status << ", errnum=" << errnum << std::endl;
}
#endif //HAVE_GETTEXTPO_XERROR

Glib::ustring get_po_context_for_item(const sharedptr<const TranslatableItem>& item, const Glib::ustring& hint)
{
  // Note that this context string should use English rather than the translated strings,
  // or the context would change depending on the locale of the user doing the export:
  Glib::ustring result = TranslatableItem::get_translatable_type_name_nontranslated(item->get_translatable_item_type());

  const Glib::ustring name = item->get_name();
  if(!name.empty())
    result += " (" + item->get_name() + ')';

  if(!hint.empty())
    result += ". " + hint;

  return result;
}

bool write_translations_to_po_file(Document* document, const Glib::ustring& po_file_uri, const Glib::ustring& translation_locale, const Glib::ustring& locale_name)
{
  std::string filename;

  try
  {
    filename = Glib::filename_from_uri(po_file_uri);
  }
  catch(const Glib::Error& ex)
  {
    std::cerr << G_STRFUNC << "Exception when converting URI to filepath: " << ex.what() << std::endl;
    return false;
  }

  if(setjmp(jump) != 0)
    return false;  

  po_file_t po_file = po_file_create();
  po_message_iterator_t msg_iter = po_message_iterator(po_file, 0);

  Document::type_list_translatables list_layout_items = document->get_translatable_items();
  for(Document::type_list_translatables::iterator iter = list_layout_items.begin(); iter != list_layout_items.end(); ++iter)
  {
    sharedptr<TranslatableItem> item = iter->first;
    if(!item)
      continue;

    if(item->get_title_original().empty())
      continue;

    const Glib::ustring hint = iter->second;

    po_message_t msg = po_message_create();
    po_message_set_msgid(msg, item->get_title_original().c_str());
    po_message_set_msgstr(msg, item->get_title_translation(translation_locale, false).c_str());

    // Add "context" comments, to uniquely identify similar strings, used in different places,
    // and to provide a hint for translators.
    const Glib::ustring msgtxt = get_po_context_for_item(item, hint);
    //std::cout << "debug: msgtxt=" << msgtxt << std::endl;
    po_message_set_msgctxt(msg, msgtxt.c_str());

    po_message_insert(msg_iter, msg);
  }

  po_message_iterator_free(msg_iter);

  #ifdef HAVE_GETTEXTPO_XERROR
  po_xerror_handler error_handler;
  memset(&error_handler, 0, sizeof(error_handler));
  error_handler.xerror = &on_gettextpo_xerror;
  error_handler.xerror2 = &on_gettextpo_xerror2;
  #else
  po_error_handler error_handler;
  memset(&error_handler, 0, sizeof(error_handler));
  error_handler.error = &on_gettextpo_error;
  #endif //HAVE_GETTEXTPO_XERROR

  output_format_po.requires_utf8 = true;
  const po_file_t written = po_file_write(po_file, filename.c_str(), &error_handler);
  po_file_free(po_file);
  
  if(!written)
    return false;

  //Prepend the po header, by reading in the data written by po_file_write(),
  //and then writing it all out again. The (generally awkward) gettext-po API 
  //does not offer an easier way to do this.
  //
  //Actually. maybe we could use po_header_set_field(), but that is not 
  //clear and that returns allocated strings that we would need to free instead 
  //of just ignoring, so this is probably still the easiest way.
  const Glib::DateTime revision_date = Glib::DateTime::create_now_local();
  const Glib::ustring revision_date_str = revision_date.format("%F %R%z");

  const Glib::ustring data = Glib::file_get_contents(filename);
  const Glib::ustring header = Glib::ustring::compose(GLOM_PO_HEADER,
    document->get_database_title(), revision_date_str, locale_name);
  const Glib::ustring full = header + "\n\n" + data;
  Glib::file_set_contents(filename, full);

  return true;
}

bool import_translations_from_po_file(Document* document, const Glib::ustring& po_file_uri, const Glib::ustring& translation_locale)
{
  std::string filename;

  try
  {
    filename = Glib::filename_from_uri(po_file_uri);
  }
  catch(const Glib::Error& ex)
  {
    std::cerr << G_STRFUNC << "Exception when converting URI to filepath: " << ex.what() << std::endl;
    return false;
  }

  Document::type_list_translatables list_layout_items = document->get_translatable_items();
  if(list_layout_items.empty())
    return false;

  if(setjmp(jump) != 0)
    return false;  

  #ifdef HAVE_GETTEXTPO_XERROR
  po_xerror_handler error_handler;
  memset(&error_handler, 0, sizeof(error_handler));
  error_handler.xerror = &on_gettextpo_xerror;
  error_handler.xerror2 = &on_gettextpo_xerror2;
  #else
  po_error_handler error_handler;
  memset(&error_handler, 0, sizeof(error_handler));
  error_handler.error = &on_gettextpo_error;
  #endif //HAVE_GETTEXTPO_XERROR

  po_file_t po_file = po_file_read(filename.c_str(), &error_handler);
  if(!po_file)
  {
    // error message is already given by error_handle.
    return false;
  }

  //Look at each domain (could there be more than one?):
  const char* const* domains = po_file_domains(po_file);
  for (int i = 0; domains[i] != 0; ++i)
  {
    //Look at each message:
    po_message_iterator_t iter = po_message_iterator(po_file, domains[i]);
    po_message_t msg;
    while ((msg = po_next_message(iter)))
    {
      //This message:
      //TODO: Just use const char* instead of copying it in to a Glib::ustring,
      //if we have performance problems here:
      const Glib::ustring msgid = Glib::convert_const_gchar_ptr_to_ustring( po_message_msgid(msg) );
      const Glib::ustring msgstr = Glib::convert_const_gchar_ptr_to_ustring( po_message_msgstr(msg) );
      const Glib::ustring msgcontext = Glib::convert_const_gchar_ptr_to_ustring( po_message_msgctxt(msg) );

      //Find the matching item in the list:
      for(Document::type_list_translatables::iterator iter = list_layout_items.begin(); iter != list_layout_items.end(); ++iter)
      {
        sharedptr<TranslatableItem> item = iter->first;
        if(!item)
          continue;

        const Glib::ustring hint = iter->second;

        if( (item->get_title_original() == msgid) && 
          (get_po_context_for_item(item, hint) == msgcontext) ) // This is not efficient, but it should be reliable.
        {
          item->set_title_translation(translation_locale, msgstr);
          // Keep examining items, in case there are duplicates. break;
        }
      }
    }

    po_message_iterator_free(iter);
  }

  po_file_free(po_file);

  document->set_modified();

  return true;
}

} //namespace Glom
