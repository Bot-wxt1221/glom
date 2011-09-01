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


#include "box_formatting.h"
#include <glom/utils_ui.h>
#include <glom/glade_utils.h>
#include <libglom/data_structure/glomconversions.h>
#include <glom/mode_design/layout/layout_item_dialogs/dialog_fieldslist.h>
#include <glibmm/i18n.h>

namespace Glom
{

const char* Box_Formatting::glade_id("box_formatting");
const bool Box_Formatting::glade_developer(true);

Box_Formatting::Box_Formatting(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
: Gtk::Box(cobject),
  m_vbox_numeric_format(0),
  m_checkbox_format_use_thousands(0),
  m_checkbox_format_use_decimal_places(0),
  m_entry_format_decimal_places(0),
  m_entry_currency_symbol(0),
  m_checkbox_format_color_negatives(0),
  m_vbox_text_format(0),
  m_combo_format_text_horizontal_alignment(0),
  m_checkbox_format_text_multiline(0),
  m_label_format_text_multiline_height(0),
  m_spinbutton_format_text_multiline_height(0),
  m_hbox_font(0),
  m_checkbox_format_text_font(0),
  m_fontbutton(0),
  m_hbox_color_foreground(0),
  m_checkbox_format_text_color_foreground(0),
  m_colorbutton_foreground(0),
  m_hbox_color_background(0),
  m_checkbox_format_text_color_background(0),
  m_colorbutton_background(0),
  m_vbox_choices(0),
  m_radiobutton_choices_custom(0),
  m_radiobutton_choices_related(0),
  m_checkbutton_choices_restricted(0),
  m_checkbutton_choices_restricted_as_radio_buttons(0),
  m_adddel_choices_custom(0),
  m_col_index_custom_choices(0),
  m_combo_choices_relationship(0),
  m_combo_choices_field(0),
  m_label_choices_extra_fields(0),
  m_button_choices_extra_fields(0),
  m_checkbutton_choices_related_show_all(0),
  m_dialog_choices_extra_fields(0),
  m_for_print_layout(false),
  m_show_numeric(true),
  m_show_editable_options(true)
{
  //Numeric formatting:
  builder->get_widget("vbox_numeric_format", m_vbox_numeric_format);
  builder->get_widget("checkbutton_format_thousands", m_checkbox_format_use_thousands);
  builder->get_widget("checkbutton_format_use_decimal_places", m_checkbox_format_use_decimal_places);
  builder->get_widget("entry_format_decimal_places", m_entry_format_decimal_places);
  builder->get_widget_derived("entry_currency_symbol", m_entry_currency_symbol);
  builder->get_widget("checkbutton_foreground_negatives", m_checkbox_format_color_negatives);

  //Text formatting:
  builder->get_widget("vbox_text_format", m_vbox_text_format);
  builder->get_widget("combo_format_text_horizontal_alignment", m_combo_format_text_horizontal_alignment);
  builder->get_widget("checkbutton_format_text_multiline", m_checkbox_format_text_multiline);
  builder->get_widget("label_format_text_multiline", m_label_format_text_multiline_height);
  builder->get_widget("spinbutton_format_text_multiline_height", m_spinbutton_format_text_multiline_height);
  builder->get_widget("hbox_font", m_hbox_font);
  builder->get_widget("checkbutton_font", m_checkbox_format_text_font);
  builder->get_widget("fontbutton", m_fontbutton);
  builder->get_widget("hbox_color_foreground", m_hbox_color_foreground);
  builder->get_widget("colorbutton_foreground", m_colorbutton_foreground);
  builder->get_widget("checkbutton_color_foreground", m_checkbox_format_text_color_foreground);
  builder->get_widget("hbox_color_background", m_hbox_color_background);
  builder->get_widget("colorbutton_background", m_colorbutton_background);
  builder->get_widget("checkbutton_color_background", m_checkbox_format_text_color_background);

  //Set the adjustment details, to avoid a useless 0-to-0 range and a 0 incremenet.
  //We don't do this the Glade file because GtkBuilder wouldn't find the
  //associated adjustment object unless we specified it explictly:
  //See http://bugzilla.gnome.org/show_bug.cgi?id=575714
  m_spinbutton_format_text_multiline_height->set_range(0, 100);
  m_spinbutton_format_text_multiline_height->set_increments(1, 10);
  m_spinbutton_format_text_multiline_height->set_value(3); //A sensible default.

  //Fill the alignment combo:
  m_model_alignment = Gtk::ListStore::create(m_columns_alignment);

  Gtk::TreeModel::iterator iter = m_model_alignment->append();
  (*iter)[m_columns_alignment.m_col_alignment] = FieldFormatting::HORIZONTAL_ALIGNMENT_AUTO;
  //Translators: This is Automatic text alignment.
  (*iter)[m_columns_alignment.m_col_title] = _("Automatic");
  iter = m_model_alignment->append();
  (*iter)[m_columns_alignment.m_col_alignment] = FieldFormatting::HORIZONTAL_ALIGNMENT_LEFT;
  //Translators: This is Left text alignment.
  (*iter)[m_columns_alignment.m_col_title] = _("Left");
  iter = m_model_alignment->append();
  (*iter)[m_columns_alignment.m_col_alignment] = FieldFormatting::HORIZONTAL_ALIGNMENT_RIGHT;
  //Translators: This is Right text alignment.
  (*iter)[m_columns_alignment.m_col_title] = _("Right");

  m_combo_format_text_horizontal_alignment->set_model(m_model_alignment);
  m_combo_format_text_horizontal_alignment->pack_start(m_columns_alignment.m_col_title);


  //Choices:
  builder->get_widget("vbox_choices", m_vbox_choices);
  builder->get_widget_derived("adddel_choices", m_adddel_choices_custom);
  m_col_index_custom_choices = m_adddel_choices_custom->add_column("Choices");
  m_adddel_choices_custom->set_allow_add();
  m_adddel_choices_custom->set_allow_delete();
  m_adddel_choices_custom->set_auto_add();

  builder->get_widget("checkbutton_choices_restrict", m_checkbutton_choices_restricted);
  builder->get_widget("checkbutton_choices_restrict_as_radio_buttons", m_checkbutton_choices_restricted_as_radio_buttons);

  builder->get_widget_derived("combobox_choices_related_relationship", m_combo_choices_relationship);
  builder->get_widget_derived("combobox_choices_related_field", m_combo_choices_field);
  builder->get_widget("label_choices_related_extra_fields", m_label_choices_extra_fields);
  builder->get_widget("button_choices_related_extra_fields", m_button_choices_extra_fields);
  builder->get_widget("checkbutton_choices_related_show_all", m_checkbutton_choices_related_show_all);
  builder->get_widget("radiobutton_choices_custom", m_radiobutton_choices_custom);
  builder->get_widget("radiobutton_choices_related", m_radiobutton_choices_related);

  m_combo_choices_relationship->signal_changed().connect(sigc::mem_fun(*this, &Box_Formatting::on_combo_choices_relationship_changed));

  m_checkbox_format_text_multiline->signal_toggled().connect( sigc::mem_fun(*this, &Box_Formatting::on_checkbox) );
  m_checkbox_format_text_font->signal_toggled().connect( sigc::mem_fun(*this, &Box_Formatting::on_checkbox) );
  m_checkbox_format_text_color_foreground->signal_toggled().connect( sigc::mem_fun(*this, &Box_Formatting::on_checkbox) );
  m_checkbox_format_text_color_background->signal_toggled().connect( sigc::mem_fun(*this, &Box_Formatting::on_checkbox) );
  m_checkbox_format_color_negatives->signal_toggled().connect( sigc::mem_fun(*this, &Box_Formatting::on_checkbox) );
  m_checkbutton_choices_restricted->signal_toggled().connect( sigc::mem_fun(*this, &Box_Formatting::on_checkbox) );
  m_button_choices_extra_fields->signal_clicked().connect( sigc::mem_fun(*this, &Box_Formatting::on_button_choices_extra) );

  if(!m_dialog_choices_extra_fields)
  {
    Utils::get_glade_widget_derived_with_warning(m_dialog_choices_extra_fields);
    add_view(m_dialog_choices_extra_fields); //Give it access to the document.
    m_dialog_choices_extra_fields->set_title(_("Extra Fields"));
  }

  show_all_children();
}

Box_Formatting::~Box_Formatting()
{
  if(m_dialog_choices_extra_fields)
  {
    remove_view(m_dialog_choices_extra_fields); //Give it access to the document.
    delete m_dialog_choices_extra_fields;
  }
}

void Box_Formatting::set_is_for_non_editable()
{
  m_for_print_layout = true;
  m_show_editable_options = false;

  //Add labels (because we will hide the checkboxes):
  Gtk::Label* label = Gtk::manage(new Gtk::Label(_("Font")));
  label->show();
  m_hbox_font->pack_start(*label, Gtk::PACK_SHRINK);
  label = Gtk::manage(new Gtk::Label(_("Foreground Color")));
  label->show();
  m_hbox_color_foreground->pack_start(*label, Gtk::PACK_SHRINK);
  label = Gtk::manage(new Gtk::Label(_("Background Color")));
  label->show();
  m_hbox_color_background->pack_start(*label, Gtk::PACK_SHRINK);

  enforce_constraints();
}

void Box_Formatting::set_formatting_for_field(const FieldFormatting& format, const Glib::ustring& table_name, const sharedptr<const Field>& field)
{
  //Used for choices and some extra text formatting:
  m_table_name = table_name;
  m_field = field;

  set_formatting_for_non_field(format,
    true /* show_numeric, ignoring anyway when m_field is set */);
}

void Box_Formatting::set_formatting_for_non_field(const FieldFormatting& format, bool show_numeric)
{
  //TODO: Split this into a private method, so that previously-set m_table_name and m_field (from set_formatting_for_field()) will not stay set.

  m_format = format;

  m_show_numeric = show_numeric;

  //Numeric formatting:
  m_checkbox_format_use_thousands->set_active( format.m_numeric_format.m_use_thousands_separator );
  m_checkbox_format_use_decimal_places->set_active( format.m_numeric_format.m_decimal_places_restricted );

  char pchText[10] = {0};
  sprintf(pchText, "%d", format.m_numeric_format.m_decimal_places);
  m_entry_format_decimal_places->set_text(Glib::ustring(pchText));

  m_entry_currency_symbol->get_entry()->set_text(format.m_numeric_format.m_currency_symbol);

  m_checkbox_format_color_negatives->set_active(
    format.m_numeric_format.m_alt_foreground_color_for_negatives );

  //Text formatting
  const FieldFormatting::HorizontalAlignment alignment =
    format.get_horizontal_alignment();
  Gtk::TreeModel::Children children = m_model_alignment->children();
  for(Gtk::TreeModel::Children::iterator iter = children.begin(); iter != children.end(); ++iter)
  {
    Gtk::TreeModel::Row row = *iter;
    if(row[m_columns_alignment.m_col_alignment] == alignment)
    {
      m_combo_format_text_horizontal_alignment->set_active(iter);
      break;
    }
  }

  m_checkbox_format_text_multiline->set_active(format.get_text_format_multiline());

  m_spinbutton_format_text_multiline_height->set_value(format.get_text_format_multiline_height_lines());

  const Glib::ustring font = format.get_text_format_font();
  m_checkbox_format_text_font->set_active(!font.empty());
  m_fontbutton->set_font_name(font);

  const Glib::ustring color_foreground = format.get_text_format_color_foreground();
  m_checkbox_format_text_color_foreground->set_active(!color_foreground.empty());
  m_colorbutton_foreground->set_color( Gdk::Color(color_foreground) );

  const Glib::ustring color_background = format.get_text_format_color_background();
  m_checkbox_format_text_color_background->set_active(!color_background.empty());
  m_colorbutton_background->set_color( Gdk::Color(color_background) );


  //Choices:
  if(m_field)
  {
    bool as_radio_buttons = false; //TODO
    m_checkbutton_choices_restricted->set_active(
      format.get_choices_restricted(as_radio_buttons));
    m_checkbutton_choices_restricted_as_radio_buttons->set_active(as_radio_buttons);

    const Document* document = get_document();

    //Fill the list of relationships:
    const Document::type_vec_relationships vecRelationships = document->get_relationships(m_table_name);
    m_combo_choices_relationship->set_relationships(vecRelationships);

    sharedptr<const Relationship> choices_relationship;
    sharedptr<const LayoutItem_Field> choices_field;
    sharedptr<const LayoutGroup> choices_field_extras;
    bool choices_show_all = false;
    format.get_choices_related(choices_relationship, choices_field, choices_field_extras, choices_show_all);

    m_combo_choices_relationship->set_selected_relationship(choices_relationship);
    on_combo_choices_relationship_changed(); //Fill the combos so we can set their active items.
    m_combo_choices_field->set_selected_field(choices_field ? choices_field->get_name() : Glib::ustring());

    //Show the list of fields in a label:
    const Glib::ustring text_extra_fields =
      Utils::get_list_of_layout_items_for_display(choices_field_extras);
     m_label_choices_extra_fields->set_text(text_extra_fields);

    //Update the contents of the dialog that will be shown if Edit is clicked:
    const Glib::ustring related_to_table =
      (choices_relationship ? choices_relationship->get_to_table() : Glib::ustring());
    if(choices_field_extras)
      m_dialog_choices_extra_fields->set_fields(related_to_table, choices_field_extras->m_list_items);
    else
      m_dialog_choices_extra_fields->set_fields(related_to_table, LayoutGroup::type_list_items());



    m_checkbutton_choices_related_show_all->set_active(choices_show_all);

    //Custom choices:
    m_adddel_choices_custom->remove_all();
    FieldFormatting::type_list_values list_choice_values = format.get_choices_custom();
    for(FieldFormatting::type_list_values::const_iterator iter = list_choice_values.begin(); iter != list_choice_values.end(); ++iter)
    {
      //Display the value in the choices list as it would be displayed in the format:
      const Glib::ustring value_text = Conversions::get_text_for_gda_value(m_field->get_glom_type(), *iter, format.m_numeric_format);
      Gtk::TreeModel::iterator iter = m_adddel_choices_custom->add_item(value_text);
      m_adddel_choices_custom->set_value(iter, m_col_index_custom_choices, value_text);
    }

    m_radiobutton_choices_custom->set_active(format.get_has_custom_choices());
    m_radiobutton_choices_related->set_active(format.get_has_related_choices());
  }

  enforce_constraints();
}

bool Box_Formatting::get_formatting(FieldFormatting& format) const
{
  //Numeric Formatting:
  m_format.m_numeric_format.m_use_thousands_separator = m_checkbox_format_use_thousands->get_active();
  m_format.m_numeric_format.m_decimal_places_restricted = m_checkbox_format_use_decimal_places->get_active();

  const Glib::ustring strDecPlaces = m_entry_format_decimal_places->get_text();
  m_format.m_numeric_format.m_decimal_places = atoi(strDecPlaces.c_str());

  m_format.m_numeric_format.m_currency_symbol = m_entry_currency_symbol->get_entry()->get_text();

  m_format.m_numeric_format.m_alt_foreground_color_for_negatives =
    m_checkbox_format_color_negatives->get_active();

  //Text formatting:
  Gtk::TreeModel::iterator iter = m_combo_format_text_horizontal_alignment->get_active();
  FieldFormatting::HorizontalAlignment alignment = FieldFormatting::HORIZONTAL_ALIGNMENT_LEFT;
  if(iter)
    alignment = (*iter)[m_columns_alignment.m_col_alignment];
  m_format.set_horizontal_alignment(alignment);

  m_format.set_text_format_multiline(m_checkbox_format_text_multiline->get_active());
  m_format.set_text_format_multiline_height_lines( m_spinbutton_format_text_multiline_height->get_value_as_int() );

  Glib::ustring font;
  if(m_checkbox_format_text_font->get_active())
    font = m_fontbutton->get_font_name();
  m_format.set_text_format_font(font);

  Glib::ustring color_foreground;
  if(m_checkbox_format_text_color_foreground->get_active())
    color_foreground = m_colorbutton_foreground->get_color().to_string();
  m_format.set_text_format_color_foreground(color_foreground);

  Glib::ustring color_background;
  if(m_checkbox_format_text_color_background->get_active())
    color_background = m_colorbutton_background->get_color().to_string();

  m_format.set_text_format_color_background(color_background);

  //Choices:
  if(m_field)
  {
    m_format.set_choices_restricted(
      m_checkbutton_choices_restricted->get_active(),
      m_checkbutton_choices_restricted_as_radio_buttons->get_active());

    const sharedptr<const Relationship> choices_relationship = m_combo_choices_relationship->get_selected_relationship();
    sharedptr<LayoutItem_Field> layout_choice_first = sharedptr<LayoutItem_Field>::create();
    layout_choice_first->set_name(m_combo_choices_field->get_selected_field_name());

    sharedptr<LayoutGroup> layout_choice_extra = sharedptr<LayoutGroup>::create();
    layout_choice_extra->m_list_items = m_dialog_choices_extra_fields->get_fields();

    m_format.set_choices_related(choices_relationship,
      layout_choice_first, layout_choice_extra,
      m_checkbutton_choices_related_show_all->get_active());

    //Custom choices:
    FieldFormatting::type_list_values list_choice_values;
    Glib::RefPtr<Gtk::TreeModel> choices_model = m_adddel_choices_custom->get_model();
    if(choices_model)
    {
      for(Gtk::TreeModel::iterator iter = choices_model->children().begin(); iter != choices_model->children().end(); ++iter)
      {
        const Glib::ustring text = m_adddel_choices_custom->get_value(iter, m_col_index_custom_choices);
        if(!text.empty())
        {
          bool success = false;
          Gnome::Gda::Value value = Conversions::parse_value(m_field->get_glom_type(), text, m_format.m_numeric_format, success);

          if(success)
            list_choice_values.push_back(value);
        }
      }
    }

    m_format.set_choices_custom(list_choice_values);
    m_format.set_has_custom_choices(m_radiobutton_choices_custom->get_active());
    m_format.set_has_related_choices(m_radiobutton_choices_related->get_active());
  }

  format = m_format;
  return true;
}

void Box_Formatting::on_combo_choices_relationship_changed()
{
  sharedptr<Relationship> relationship = m_combo_choices_relationship->get_selected_relationship();

  Document* pDocument = get_document();
  if(pDocument)
  {
    //Show the list of formats from this relationship:
    if(relationship)
    {
      const Document::type_vec_fields vecFields = pDocument->get_table_fields(relationship->get_to_table());
      m_combo_choices_field->set_fields(vecFields);

      //Update the show-all dialog's list:
      //If the related table name has changed then the list of fields will probably
      //be ignored, clearing the list, but we try to preserve it if possible:
      const LayoutGroup::type_list_items list_fields = m_dialog_choices_extra_fields->get_fields();
      m_dialog_choices_extra_fields->set_fields(relationship->get_to_table(), list_fields);

      //Update the label:
      const Glib::ustring text_extra_fields =
        Utils::get_list_of_layout_items_for_display(m_dialog_choices_extra_fields->get_fields());
       m_label_choices_extra_fields->set_text(text_extra_fields);
    }
  }
}

void Box_Formatting::enforce_constraints()
{
  //Hide inappropriate UI:

  bool show_text = true;
  if(m_field)
  {
    m_show_numeric = false;
    if(m_field->get_glom_type() == Field::TYPE_NUMERIC)
      m_show_numeric = true;

    if((m_field->get_glom_type() == Field::TYPE_BOOLEAN) || (m_field->get_glom_type() == Field::TYPE_IMAGE)) //TODO: Allow text options when showing booleans as Yes/No on print layouts.
    {
      show_text = false;
    }
  }

  if(show_text)
  {
    m_vbox_text_format->show();

    //Hide multiline options for non-text fields:
    if(m_for_print_layout || !m_field || (m_field->get_glom_type() != Field::TYPE_TEXT))
    {
      m_checkbox_format_text_multiline->hide();
      m_label_format_text_multiline_height->hide();
      m_spinbutton_format_text_multiline_height->hide();
    }
    else
    {
      m_checkbox_format_text_multiline->show();
      m_label_format_text_multiline_height->hide();
      m_spinbutton_format_text_multiline_height->show();
    }

    //Do not allow the user to specify a text height if it is not going to be multiline:
    const bool text_height_sensitive = m_checkbox_format_text_multiline->get_active();
    m_spinbutton_format_text_multiline_height->set_sensitive(text_height_sensitive);
  }
  else
    m_vbox_text_format->hide();


  //Hide the checkbuttons (we show labels instead) for print layouts:
  if(m_for_print_layout)
  {
    m_checkbox_format_text_font->hide();
    m_checkbox_format_text_color_background->hide();
    m_checkbox_format_text_color_foreground->hide();

    //But enable them so that other code, that checks for it, works:
    m_checkbox_format_text_font->set_active();
    m_checkbox_format_text_color_background->set_active();
    m_checkbox_format_text_color_foreground->set_active();
  }

  //Enable UI depending on the checkbutton state:
  m_fontbutton->set_sensitive( m_checkbox_format_text_font->get_active() );
  m_colorbutton_foreground->set_sensitive( m_checkbox_format_text_color_foreground->get_active() );
  m_colorbutton_background->set_sensitive(  m_checkbox_format_text_color_background->get_active() );

  //Choices:
  //Radio buttons only make sense when the items are restricted, instead of free-form:
  m_checkbutton_choices_restricted_as_radio_buttons->set_sensitive(
     m_checkbutton_choices_restricted->get_active());

  if(m_show_numeric)
    m_vbox_numeric_format->show();
  else
    m_vbox_numeric_format->hide();

  if(m_field && m_show_editable_options)
    m_vbox_choices->show();
  else
    m_vbox_choices->hide();
}

void Box_Formatting::on_checkbox()
{
  enforce_constraints();
}

void Box_Formatting::on_button_choices_extra()
{
  if(!m_dialog_choices_extra_fields)
    return;

  const int response = Glom::Utils::dialog_run_with_help(m_dialog_choices_extra_fields);
  m_dialog_choices_extra_fields->hide();
  if(response == Gtk::RESPONSE_OK && m_dialog_choices_extra_fields->get_modified())
  {
    //Update the label:
    const Glib::ustring text_extra_fields =
      Utils::get_list_of_layout_items_for_display(m_dialog_choices_extra_fields->get_fields());
     m_label_choices_extra_fields->set_text(text_extra_fields);

    //Tell the parent (which connects directly to all other (regular) widgets):
    m_signal_modified.emit();
  }
}


Box_Formatting::type_signal_modified Box_Formatting::signal_modified()
{
  return m_signal_modified;
}

} //namespace Glom
