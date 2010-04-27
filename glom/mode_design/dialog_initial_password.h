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

#ifndef GLOM_DIALOG_NEWSELFHOSTEDCONNECTION_H
#define GLOM_DIALOG_NEWSELFHOSTEDCONNECTION_H

#include <gtkmm/builder.h>
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/label.h>
#include <glom/base_db.h>
#include <libglom/connectionpool.h>

namespace Glom
{

class Dialog_InitialPassword
  : public Gtk::Dialog,
    public Base_DB
{
public:
  static const char* glade_id;
  static const bool glade_developer;

  Dialog_InitialPassword(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
  virtual ~Dialog_InitialPassword();

  bool check_password();

  Glib::ustring get_user() const;
  Glib::ustring get_password() const;

  virtual void load_from_document(); //override


private:

  Gtk::Entry* m_entry_user;
  Gtk::Entry* m_entry_password;
  Gtk::Entry* m_entry_password_confirm;
};

} //namespace Glom

#endif //GLOM_DIALOG_NEWSELFHOSTEDCONNECTION_H

