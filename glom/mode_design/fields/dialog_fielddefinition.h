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

#ifndef GLOM_MODE_DESIGN_DIALOG_FIELDDEFINITION_H
#define GLOM_MODE_DESIGN_DIALOG_FIELDDEFINITION_H

#include <gtkmm.h>
#include "../../utility_widgets/combo_textglade.h"
#include <glom/mode_design/layout/combobox_relationship.h>
#include "combo_fieldtype.h"
//#include "../../utility_widgets/entry_numerical.h"
#include "../../utility_widgets/dialog_properties.h"
#include <glom/mode_data/datawidget/datawidget.h>
#include <libglom/data_structure/field.h>
#include <glom/mode_design/layout/layout_item_dialogs/box_formatting.h>
#include <glom/base_db.h>
#include <gtksourceviewmm/view.h>

namespace Glom
{

class Dialog_FieldDefinition
 : public Dialog_Properties,
   public Base_DB //Give this class access to the current document, and to some utility methods.
{
public:
  static const char* glade_id;
  static const bool glade_developer;

  Dialog_FieldDefinition(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
  virtual ~Dialog_FieldDefinition();

  virtual void set_field(const sharedptr<const Field>& field, const Glib::ustring& table_name);
  virtual sharedptr<Field> get_field() const; //TODO_FieldShared

private:

  //Signal handlers:
  void on_combo_type_changed();
  void on_combo_lookup_relationship_changed();
  void on_check_lookup_toggled();
  void on_radio_calculate_toggled();
  void on_radio_userentry_toggled();
  void on_button_edit_calculation();

  //void on_foreach(Gtk::Widget& widget);

  //Disable/enable other controls when a control is selected.
  virtual void enforce_constraints(); //override.

  Gtk::Entry* m_pEntry_Name;
  Combo_FieldType* m_pCombo_Type;
  Gtk::CheckButton* m_pCheck_Unique;
  Gtk::CheckButton* m_pCheck_NotNull;
  Gtk::Box* m_pBox_DefaultValueSimple;
  Gtk::CheckButton* m_pCheck_PrimaryKey;
  Gtk::CheckButton* m_pCheck_AutoIncrement;

  Gtk::Box* m_pBox_ValueTab;

  Gtk::RadioButton* m_pRadio_UserEntry;
  Gtk::Alignment* m_pAlignment_UserEntry;
  Gtk::CheckButton* m_pCheck_Lookup;
  Gtk::Table* m_pTable_Lookup;
  ComboBox_Relationship* m_pCombo_LookupRelationship;
  Combo_TextGlade* m_pCombo_LookupField;

  Gtk::RadioButton* m_pRadio_Calculate;
  Gtk::Alignment* m_pAlignment_Calculate;
  Gsv::View* m_pTextView_Calculation;
  Gtk::Button* m_pButton_EditCalculation;

  Gtk::Entry* m_pEntry_Title;

  DataWidget* m_pDataWidget_DefaultValueSimple;

  Gtk::Box* m_box_formatting_placeholder;
  Box_Formatting* m_box_formatting;

  sharedptr<Field> m_Field;
  Glib::ustring m_table_name;
};

} //namespace Glom

#endif // GLOM_MODE_DESIGN_DIALOG_FIELDDEFINITION_H
