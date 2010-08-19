/*
 * Copyright 2000 Murray Cumming
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

#ifndef DIALOG_OFFERSAVE_H
#define DIALOG_OFFERSAVE_H

#include "config.h"

#ifdef GLOM_ENABLE_MAEMO
#include <hildonmm/note.h>
#else
#include <gtkmm/messagedialog.h>
#endif

namespace GlomBakery
{

#ifdef GLOM_ENABLE_MAEMO
class Dialog_OfferSave : public Hildon::Note
#else
class Dialog_OfferSave : public Gtk::MessageDialog
#endif
{
public:
  Dialog_OfferSave(const Glib::ustring& file_uri);
  virtual ~Dialog_OfferSave();

  ///Return values:
  enum enumButtons
  {
    BUTTON_Save,
    BUTTON_Discard,
    BUTTON_Cancel
  };

};

} //namespace

#endif //DIALOG_OFFERSAVE_H
