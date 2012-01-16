/* Glom
 *
 * Copyright (C) 2001-2009 Murray Cumming
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

#ifndef GLOM_DOCUMENT_H
#define GLOM_DOCUMENT_H

#include <libglom/libglom_config.h>

#include <libglom/document/bakery/document_xml.h>
#include <libglom/data_structure/relationship.h>
#include <libglom/data_structure/field.h>
#include <libglom/data_structure/layout/layoutgroup.h>
#include <libglom/data_structure/layout/layoutitem_notebook.h>
#include <libglom/data_structure/layout/layoutitem_portal.h>
#include <libglom/data_structure/layout/layoutitem_button.h>
#include <libglom/data_structure/layout/layoutitem_text.h>
#include <libglom/data_structure/layout/layoutitem_image.h>
#include <libglom/data_structure/tableinfo.h>
#include <libglom/data_structure/groupinfo.h>
#include <libglom/data_structure/report.h>
#include <libglom/data_structure/print_layout.h>
#include <libglom/data_structure/foundset.h>
#include <libglom/appstate.h>
#include <vector>
#include <map>
#include <limits> // for numeric_limits

namespace Gtk
{
class Window;
}

namespace Glom
{

class Document : public GlomBakery::Document_XML
{
public:
  Document();
  virtual ~Document();

  virtual void set_modified(bool value = true);

  /** Set the file URI that will be used in future calls to load() and save().
   * Note that the document will not be saved immediately to the new URI. It will
   * be saved either after the next change (if using autosave) or when calling save() explicitly.
   * Likewise, the document at the URI will not be loaded until load() is called explicitly.
   * That is unlike in the base class's implementation.
   */
  virtual void set_file_uri(const Glib::ustring& file_uri, bool bEnforceFileExtension = false);

  /* Loads data from disk, using the URI (set with set_file_uri()) then asks the View to update itself.
   * bool indicates success.
   * This is just here so the SWIG Java API generator does not need to wrap methods from the base classes.
   */
  bool load(int& failure_code);


  /** Whether the document was opened from another networked glom instance,
   * instead of via a URI.
   */
  void set_opened_from_browse(bool val = true);
  bool get_opened_from_browse() const;

  /** The document usually saves itself when you call set_modified().
   * Pass false to this function to prevent that temporarily.
   * The document will be saved, if necessary, after you call this function with true.
   */
  void set_allow_autosave(bool value = true);

  bool get_is_example_file() const;
  void set_is_example_file(bool value = true);
  
  bool get_is_backup_file() const;
  void set_is_backup_file(bool value = true);

  /* Get version of the document format used for this document.
   *  This can increase when the file has been re-saved.
   *  See get_latest_known_document_format_version().
   *  Old versions of the application cannot open documents with a newer document format,
   *  so saving with a version of the application that has a newer document format will
   *  make it impossible to open the document in a version of the application with an older document format.
   */
  guint get_document_format_version();

  static guint get_latest_known_document_format_version();

  /// How the database is hosted.
  enum HostingMode
  {
    HOSTING_MODE_POSTGRES_CENTRAL, /*!< The database is hosted on an external postgresql server. */
    HOSTING_MODE_POSTGRES_SELF, /*!< A new postgres database process is spawned that hosts the data. */
    HOSTING_MODE_SQLITE, /*!< A sqlite database file is used. */
    HOSTING_MODE_DEFAULT = HOSTING_MODE_POSTGRES_SELF /*!- Arbitrary default. */
  };

  /** Set the hosting mode of the database.
   */
  void set_hosting_mode(HostingMode mode);

  /** This returns how the database is hosted.
   */
  HostingMode get_hosting_mode() const;

  /** Whether the database (and document) is shared over the network.
   * This setting is saved in the file, allowing the database to be
   * shared immediately after opening the document.
   * @param shared true if the database should be shared.
   */
  void set_network_shared(bool shared = true);

  /** See set_network_shared().
   * @result true if the database is (or should be) shared over the network.
   */
  bool get_network_shared() const;

  void set_connection_server(const Glib::ustring& strVal);
  void set_connection_database(const Glib::ustring& strVal);
  void set_connection_port(unsigned int port_number);
  void set_connection_try_other_ports(bool val);

 /** Temporarily set a username in the document.
   * Note that this is not saved in the document's file.
   *
   * TODO: Remove this, and just store it in ConnectionPool?
   */
  void set_connection_user(const Glib::ustring& strVal);

  /** If the database should be hosted, this provides the
    * path to the directory that contains all the files needed to do that.
    * This is usually a specifically-named directory at the same level as the .glom file.
    * If the database is a sqlite database, this specifies the directory in
    * which the database file is in.
    */
  std::string get_connection_self_hosted_directory_uri() const;

  Glib::ustring get_connection_server() const;
  Glib::ustring get_connection_database() const;
  unsigned int get_connection_port() const;
  bool get_connection_try_other_ports() const;

  /** Retrieve a username previously set in the document.
   * Note that this is not saved in the document's file.
   *
   * TODO: Remove this, and just store it in ConnectionPool?
   */
  Glib::ustring get_connection_user() const;

  /** Set the language/locale used by original titles.
   * Title translations are translations of the text in this language.
   * @param locale: For instance, "en_US.UTF-8".
   */
  void set_translation_original_locale(const Glib::ustring& locale);

  /** Get the language/locale used by original titles.
   * Title translations are translations of the text in this language.
   */
  Glib::ustring get_translation_original_locale() const;

  /** Get a list of locales for which at least one string is translated.
   * The result will include the original, from get_translation_original_locale().
   */
  std::vector<Glib::ustring> get_translation_available_locales() const;

  typedef std::vector< sharedptr<Relationship> > type_vec_relationships;
  type_vec_relationships get_relationships(const Glib::ustring& table_name, bool plus_system_prefs = false) const;
  void set_relationships(const Glib::ustring& table_name, const type_vec_relationships& vecRelationships);

  sharedptr<Relationship> get_relationship(const Glib::ustring& table_name, const Glib::ustring& relationship_name) const;
  void set_relationship(const Glib::ustring& table_name, const sharedptr<Relationship>& relationship);

  void remove_relationship(const sharedptr<const Relationship>& relationship);

  /** Returns whether the relationship's to-field is a primary key  or unique field, meaning
   * that there can be only one related record for each value of the from-field.
   */
  bool get_relationship_is_to_one(const Glib::ustring& table_name, const Glib::ustring& relationship_name) const;

  /** Returns whether the field is the from-field in a to-one relationship.
   * @see get_relationship_is_to_one(). Ignores hidden tables.
   */
  sharedptr<const Relationship> get_field_used_in_relationship_to_one(const Glib::ustring& table_name, const sharedptr<const LayoutItem_Field>& layout_field) const;

  typedef std::vector< sharedptr<Field> > type_vec_fields;
  type_vec_fields get_table_fields(const Glib::ustring& table_name) const;
  void set_table_fields(const Glib::ustring& table_name, const type_vec_fields& vecFields);

  sharedptr<Field> get_field(const Glib::ustring& table_name, const Glib::ustring& strFieldName) const;

  sharedptr<Field> get_field_primary_key(const Glib::ustring& table_name) const;

  /** Use this after removing a field from a table,
   * so that it is not used anymore in relationships, layouts, reports, etc.
   */
  void remove_field(const Glib::ustring& table_name, const Glib::ustring& field_name);
  
  
  typedef std::pair< sharedptr<LayoutItem_Field>, sharedptr<Relationship> > type_pairFieldTrigger;
  typedef std::list<type_pairFieldTrigger> type_list_lookups;
  
  /** Get the fields whose values should be looked up when @a field_name changes, with
   * the relationship used to lookup the value.
   */
  type_list_lookups get_lookup_fields(const Glib::ustring& table_name, const Glib::ustring& field_name) const;



  typedef std::vector< sharedptr<LayoutGroup> > type_list_layout_groups;

  /** Get the layout groups for a layout.
   * @param layout_name The name of the layout, such as list or details.
   * @param parent_table_name The name of the table for which this layout should appear.
   * @param layout_platform The platform for which this layout should be used. Possible values are an empty string (meaning normal platforms) or "maemo" meaning "normal".
   * @result A list of layout groups at the top-level of the requested layout.
   */
  type_list_layout_groups get_data_layout_groups(const Glib::ustring& layout_name, const Glib::ustring& parent_table_name, const Glib::ustring& layout_platform = Glib::ustring()) const;

  /** Discover whether there are any fields in the layout.
   * @param layout_name The name of the layout, such as list or details.
   * @param parent_table_name The name of the table for which this layout should appear.
   * @param layout_platform The platform for which this layout should be used. Possible values are an empty string (meaning normal platforms) or "maemo" meaning "normal".
   * @result true if there is at least one field in the layout group or its sub groups.
   */
  bool get_data_layout_groups_have_any_fields(const Glib::ustring& layout_name, const Glib::ustring& parent_table_name, const Glib::ustring& layout_platform = Glib::ustring()) const;

  /** Set the layout groups for a layout.
   * @param layout_name The name of the layout, such as list or details.
   * @param parent_table_name The name of the table for which this layout should appear.
   * @param layout_platform The platform for which this layout should be used. Possible values are an empty string (meaning normal platforms) or "maemo" meaning "normal".
   * @param groups A list of layout groups at the top-level of the requested layout.
   */
  void set_data_layout_groups(const Glib::ustring& layout_name, const Glib::ustring& parent_table_name, const Glib::ustring& layout_platform, const type_list_layout_groups& groups);

  /**
   * @para The layout_name, such as "details", "list".
   * @para parent_table_name The name of the table on whose layout the layout appears.
   * @param layout_platform The platform for which this layout should be used. Possible values are an empty string (meaning normal platforms) or "maemo" meaning "normal".
   */
  type_list_layout_groups get_data_layout_groups_plus_new_fields(const Glib::ustring& layout_name, const Glib::ustring& parent_table_name, const Glib::ustring& layout_platform = Glib::ustring()) const;

  type_list_layout_groups get_data_layout_groups_default(const Glib::ustring& layout_name, const Glib::ustring& parent_table_name, const Glib::ustring& layout_platform = Glib::ustring()) const;

  /// The translatable item and a hint about what it is.
  typedef std::pair< sharedptr<TranslatableItem>, Glib::ustring> pair_translatable_item_and_hint;

  typedef std::list<pair_translatable_item_and_hint> type_list_translatables;
  type_list_translatables get_translatable_items();

  static void fill_translatable_custom_choices(FieldFormatting& formatting, type_list_translatables& the_list, const Glib::ustring& hint);


  void fill_layout_field_details(const Glib::ustring& parent_table_name, const sharedptr<LayoutGroup>& layout_group) const;
  void fill_layout_field_details(const Glib::ustring& parent_table_name, type_list_layout_groups& groups) const;

  ///When a field name is changed, change it in the relationships, layouts, reports, and fields data:
  void change_field_name(const Glib::ustring& table_name, const Glib::ustring& strFieldNameOld, const Glib::ustring& strFieldNameNew);

  ///When a table name is changed, change it in the relationships and tables data:
  void change_table_name(const Glib::ustring& table_name_old, const Glib::ustring& table_name_new);

  ///When a relationship name is changed, change it in layouts and reports:
  void change_relationship_name(const Glib::ustring& table_name, const Glib::ustring& name, const Glib::ustring& name_new);

  typedef std::list< sharedptr<TableInfo> > type_listTableInfo;
  type_listTableInfo get_tables(bool plus_system_prefs = false) const;
  std::vector<Glib::ustring> get_table_names(bool plus_system_prefs = false) const;

  void set_tables(const type_listTableInfo& tables);

  sharedptr<TableInfo> get_table(const Glib::ustring& table_name) const;
  void add_table(const sharedptr<TableInfo>& table_name);

  /** Use this after DROPing the table.
   * It removes information about the table, including fields and layouts,
   * and any place that parts of the table are used.
   */
  void remove_table(const Glib::ustring& table_name);

  bool get_table_is_known(const Glib::ustring& table_name) const;
  bool get_table_is_hidden(const Glib::ustring& table_name) const;

  Glib::ustring get_table_title(const Glib::ustring& table_name, const Glib::ustring& locale) const;
  Glib::ustring get_table_title_original(const Glib::ustring& table_name) const;
  void set_table_title(const Glib::ustring& table_name, const Glib::ustring& value, const Glib::ustring& locale);

  Glib::ustring get_table_title_singular(const Glib::ustring& table_name, const Glib::ustring& locale) const;
  Glib::ustring get_table_title_singular_original(const Glib::ustring& table_name) const;

  typedef std::vector< Gnome::Gda::Value > type_row_data;
  typedef std::vector< type_row_data > type_example_rows;

  /** Save example data into the document, for use when creating the example database on the server.
   * Don't use this for large amounts of data.
   * @param table_name The table that should contain this example data.
   * @param rows Each row is separated by a newline. Each line has comma-separated field values, in SQL format.
   */
  void set_table_example_data(const Glib::ustring& table_name, const type_example_rows& rows);

  type_example_rows get_table_example_data(const Glib::ustring& table_name) const;

  virtual Glib::ustring get_name() const; //override.

  Glib::ustring get_default_table() const;
  Glib::ustring get_first_table() const;

  Glib::ustring get_database_title() const;
  void set_database_title(const Glib::ustring& title);

  std::vector<Glib::ustring> get_library_module_names() const;
  void set_library_module(const Glib::ustring& name, const Glib::ustring& script);
  Glib::ustring get_library_module(const Glib::ustring& name) const;
  void remove_library_module(const Glib::ustring& name);

  /** Get a Python script that should be run when the document is opened.
   */
  Glib::ustring get_startup_script() const;

  /** See get_startup_script().
   */
  void set_startup_script(const Glib::ustring& script);

  /// These are only used when recreating a database from an example file. The actualy access-control is on the server, of course.
  typedef std::list<GroupInfo> type_list_groups;
  type_list_groups get_groups() const;

  /// This adds the group if necessary.
  void set_group(GroupInfo& group);

  void remove_group(const Glib::ustring& group_name);


  typedef std::list<Glib::ustring> type_listReports;
  type_listReports get_report_names(const Glib::ustring& table_name) const;

  void set_report(const Glib::ustring& table_name, const sharedptr<Report>& report);
  sharedptr<Report> get_report(const Glib::ustring& table_name, const Glib::ustring& report_name) const;
  void remove_report(const Glib::ustring& table_name, const Glib::ustring& report_name);

  //Print Layouts are precisely positioned layouts for printing to a printer:
  typedef std::list<Glib::ustring> type_listPrintLayouts;
  type_listPrintLayouts get_print_layout_names(const Glib::ustring& table_name) const;
  void set_print_layout(const Glib::ustring& table_name, const sharedptr<PrintLayout>& print_layout);
  sharedptr<PrintLayout> get_print_layout(const Glib::ustring& table_name, const Glib::ustring& print_layout_name) const;
  void remove_print_layout(const Glib::ustring& table_name, const Glib::ustring& print_layout_name);

  void set_layout_record_viewed(const Glib::ustring& table_name, const Glib::ustring& layout_name, const Gnome::Gda::Value& primary_key_value);
  void forget_layout_record_viewed(const Glib::ustring& table_name);
  Gnome::Gda::Value get_layout_record_viewed(const Glib::ustring& table_name, const Glib::ustring& layout_name) const;

  //TODO: Rename set_layout_current() and set_criteria_current().

  /** Temporarily save (but not in the document) the last-viewed layout for the table,
   * so we can show the same layout when navigating back to this table later.
   *
   * @param table_name The table.
   * @param layout_name The layout name, such as "list" or "details".
   */
  void set_layout_current(const Glib::ustring& table_name, const Glib::ustring& layout_name);

  /** Temporarily save (but not in the document) the last-viewed criteria for the table,
   * so we can show the same criteria (sort order, where clause) when navigating back to this table later.
   *
   * @param table_name The table.
   * @param found_set Additional information about the last use of that layout, such as the sort order or where clause.
   */
  void set_criteria_current(const Glib::ustring& table_name,const FoundSet& found_set);

  Glib::ustring get_layout_current(const Glib::ustring& table_name) const;

  FoundSet get_criteria_current(const Glib::ustring& table_name) const;

#ifndef SWIG //Hide this API from swig.
  // Used by Relationship Overview dialog to preserve table locations accross instantiations:

  /** Retrieve the x and y coordinates for the given table position in the relationship overview dialog.
   *
   * @param table_name The name of the table to query.
   * @param x The x coordinate of the table position.
   * @param y The y coordinate of the table position.
   * @return false if the table does not have any position for this table.
   */
  bool get_table_overview_position( const Glib::ustring& table_name, float &x, float &y ) const;

  /** Set the position of a table in the relationship overview dialog.
   *
   * @param table_name The name of the table to modify.
   * @param x The x coordinate of the table position.
   * @param y The y coordinate of the table position.
   */
  void set_table_overview_position( const Glib::ustring& utable_name, float x, float y );

  enum userLevelReason
  {
    USER_LEVEL_REASON_UNKNOWN,
    USER_LEVEL_REASON_FILE_READ_ONLY,
    USER_LEVEL_REASON_DATABASE_ACCESS_LEVEL,
    USER_LEVEL_REASON_OPENED_FROM_BROWSE
  };

  /**
   * @param reason The reason that the user is not a developer, if he is not.
   * @result Whether the user is a developer.
   */
  AppState::userlevels get_userlevel(userLevelReason& reason) const;

  AppState::userlevels get_userlevel() const;

  /** This is transitory information, not saved to disk.
   */
  bool set_userlevel(AppState::userlevels userlevel);

  typedef sigc::signal<void, AppState::userlevels> type_signal_userlevel_changed;
  type_signal_userlevel_changed signal_userlevel_changed();

  //TODO: This is a rather indirect way for application.cc to request the UI to update for the userlevel.
  void emit_userlevel_changed();
#endif //SWIG

  /** This is transitory information, not saved to disk.
   */
  Glib::ustring get_active_layout_platform() const;

  /** This is transitory information, not saved to disk.
   */
  void set_active_layout_platform(const Glib::ustring& layout_platform = Glib::ustring());

#ifndef SWIG //Hide this API from swig.

  Glib::ustring build_and_get_contents() const;

  /** This callback should show UI to indicate that work is still happening.
   * For instance, a pulsing ProgressBar.
   */
  typedef sigc::slot<void> SlotProgress;
  
  /** Save a copy of the document as a backup.
   * This document (and its URI) will not be changed.
   * @param The location at which to save the backup Glom file.
   * @result The URI of the .tar.gz tarball.
   */
  Glib::ustring save_backup_file(const Glib::ustring& uri, const SlotProgress& slot_progress);
  
  /**
   * @param backup_uri: The URI of a .tar.gz backup file.
   * @result The URI of the restored .glom file.
   */
  static Glib::ustring restore_backup_file(const Glib::ustring& backup_uri, const SlotProgress& slot_progress);
  

protected:


  //TODO: Remove this:
  friend class ReportBuilder;

  ///If value is equal to the default then no attribute will be set, to save text space in the XML file.
  static void set_node_attribute_value_as_decimal(xmlpp::Element* node, const Glib::ustring& strAttributeName, guint value, guint value_default = 0);

  // This is required by the report builder, so it cannot be disabled
  // in client only mode
  static void set_node_attribute_value_as_decimal_double(xmlpp::Element* node, const Glib::ustring& strAttributeName, double value);
#endif //SWIG

#ifndef SWIG
public:
  static sharedptr<TableInfo> create_table_system_preferences();
  static sharedptr<TableInfo> create_table_system_preferences(type_vec_fields& fields);
  static sharedptr<Relationship> create_relationship_system_preferences(const Glib::ustring& table_name);
  static bool get_relationship_is_system_properties(const sharedptr<const Relationship>& relationship);
#endif //SWIG

  /// Failure codes that could be returned by load_after()
  enum load_failure_codes
  {
    LOAD_FAILURE_CODE_FILE_VERSION_TOO_NEW = LOAD_FAILURE_CODE_LAST + 1
  };

private:
  //Overrides:

  virtual bool save_before();
  void save_before_layout_group(xmlpp::Element* node, const sharedptr<const LayoutGroup>& group, bool with_print_layout_positions = false);
  void save_before_sort_by(xmlpp::Element* node, const LayoutItem_GroupBy::type_list_sort_fields& list_fields);
  void save_before_layout_item_usesrelationship(xmlpp::Element* nodeItem, const sharedptr<const UsesRelationship>& item);
  void save_before_layout_item_field(xmlpp::Element* nodeItem, const sharedptr<const LayoutItem_Field>& item);
  void save_before_layout_item_formatting(xmlpp::Element* nodeItem, const FieldFormatting& format, Field::glom_field_type field_type = Field::TYPE_INVALID);
  void save_before_layout_item_formatting(xmlpp::Element* nodeItem, const sharedptr<const LayoutItem_WithFormatting>& layout_item);

  void save_before_translations(xmlpp::Element* nodeItem, const TranslatableItem& item);
  void save_before_print_layout_position(xmlpp::Element* nodeItem, const sharedptr<const LayoutItem>& item);
  void save_before_choicevalue(xmlpp::Element* nodeItem, const sharedptr<const ChoiceValue>& item, Field::glom_field_type field_type);

  void save_changes();

  virtual bool load_after(int& failure_code);
  void load_after_layout_group(const xmlpp::Element* node, const Glib::ustring& table_name, const sharedptr<LayoutGroup>& group, bool with_print_layout_positions = false);
  void load_after_sort_by(const xmlpp::Element* node, const Glib::ustring& table_name, LayoutItem_GroupBy::type_list_sort_fields& list_fields);
  void load_after_layout_item_usesrelationship(const xmlpp::Element* element, const Glib::ustring& table_name, const sharedptr<UsesRelationship>& item);
  void load_after_layout_item_field(const xmlpp::Element* element, const Glib::ustring& table_name, const sharedptr<LayoutItem_Field>& item);
  void load_after_layout_item_formatting(const xmlpp::Element* element, FieldFormatting& format, Field::glom_field_type field_type = Field::TYPE_INVALID, const Glib::ustring& table_name = Glib::ustring(), const Glib::ustring& field_name = Glib::ustring());
 void load_after_layout_item_formatting(const xmlpp::Element* element, const sharedptr<LayoutItem_WithFormatting>& layout_item, const Glib::ustring& table_name = Glib::ustring());

  void load_after_translations(const xmlpp::Element* element, TranslatableItem& item);
  void load_after_print_layout_position(const xmlpp::Element* nodeItem, const sharedptr<LayoutItem>& item);
  void load_after_choicevalue(const xmlpp::Element* element, const sharedptr<ChoiceValue>& item, Field::glom_field_type field_type);

  void on_app_state_userlevel_changed(AppState::userlevels userlevel);

  static void fill_translatable_layout_items(const sharedptr<LayoutItem_Field>& layout_field, type_list_translatables& the_list, const Glib::ustring& hint);
  static void fill_translatable_layout_items(const sharedptr<LayoutGroup>& group, type_list_translatables& the_list, const Glib::ustring& hint);

  void fill_sort_field_details(const Glib::ustring& parent_table_name, FieldFormatting::type_list_sort_fields& sort_fields) const;

  type_list_translatables get_translatable_layout_items(const Glib::ustring& table_name, const Glib::ustring& hint);
  type_list_translatables get_translatable_report_items(const Glib::ustring& table_name, const Glib::ustring& report_title, const Glib::ustring& hint);


  /// If the attribute is not there, then the default will be returned.
  static bool get_node_attribute_value_as_bool(const xmlpp::Element* node, const Glib::ustring& strAttributeName, bool value_default = false);
  static guint get_node_attribute_value_as_decimal(const xmlpp::Element* node, const Glib::ustring& strAttributeName, guint value_default = 0);
  static double get_node_attribute_value_as_decimal_double(const xmlpp::Element* node, const Glib::ustring& strAttributeName);
  static float get_node_attribute_value_as_float(const xmlpp::Element* node, const Glib::ustring& strAttributeName);

  static void set_node_attribute_value_as_bool(xmlpp::Element* node, const Glib::ustring& strAttributeName, bool value = true, bool value_default = false);
  static void set_node_attribute_value_as_float( xmlpp::Element* node, const Glib::ustring& strAttributeName, float value );
  static void set_node_attribute_value_as_value(xmlpp::Element* node, const Glib::ustring& strAttributeName, const Gnome::Gda::Value& value, Field::glom_field_type field_type);
  static void set_node_text_child_as_value(xmlpp::Element* node, const Gnome::Gda::Value& value, Field::glom_field_type field_type);

  static Gnome::Gda::Value get_node_attribute_value_as_value(const xmlpp::Element* node, const Glib::ustring& strAttributeName, Field::glom_field_type field_type);
  static Gnome::Gda::Value get_node_text_child_as_value(const xmlpp::Element* node, Field::glom_field_type field_type);

  Glib::ustring get_child_text_node(const xmlpp::Element* node, const Glib::ustring& child_node_name) const;
  void set_child_text_node(xmlpp::Element* node, const Glib::ustring& child_node_name, const Glib::ustring& text);

  AppState m_app_state;
  type_signal_userlevel_changed m_signal_userlevel_changed;

  HostingMode m_hosting_mode;
  bool m_network_shared;

  Glib::ustring m_connection_server, m_connection_database;
  Glib::ustring m_connection_user; //Don't save the user.
  unsigned int m_connection_port; //0 means any port. Ignored when self-hosting (which may use a different port each time).
  bool m_connection_try_other_ports; //Set to false for self-hosted or browsed-from-network documents.

  class LayoutInfo
  {
  public:
    Glib::ustring m_layout_name;
    Glib::ustring m_layout_platform; //Empty string (meaning normal platforms), or "maemo", or something else.
    Glib::ustring m_parent_table;

    type_list_layout_groups m_layout_groups;
  };

  class DocumentTableInfo
  {
  public:
    DocumentTableInfo()
      : m_overviewx ( std::numeric_limits<float>::infinity () ),
        m_overviewy ( std::numeric_limits<float>::infinity () )
    {
      m_info = sharedptr<TableInfo>(new TableInfo()); //Avoid a null sharedptr.
    }

    //TODO: Avoid the use of this:
    DocumentTableInfo(const DocumentTableInfo& src)
      : m_info(src.m_info),
        m_fields(src.m_fields),
        m_relationships(src.m_relationships),
        m_layouts(src.m_layouts),
        m_reports(src.m_reports),
        m_print_layouts(src.m_print_layouts),
        m_example_rows(src.m_example_rows),
        m_map_current_record(src.m_map_current_record),
        m_layout_current(src.m_layout_current),
        m_foundset_current(src.m_foundset_current),
        m_overviewx(src.m_overviewx),
        m_overviewy(src.m_overviewy)
    {
    }

    //TODO: Avoid the use of this:
    DocumentTableInfo& operator=(const DocumentTableInfo& src)
    {
      m_info = src.m_info;
      m_fields = src.m_fields;
      m_relationships = src.m_relationships;
      m_layouts = src.m_layouts;
      m_reports = src.m_reports;
      m_print_layouts = src.m_print_layouts;
      m_example_rows = src.m_example_rows;
      m_map_current_record = src.m_map_current_record;
      m_layout_current = src.m_layout_current;
      m_foundset_current = src.m_foundset_current;
      m_overviewx = src.m_overviewx;
      m_overviewy = src.m_overviewy;

      return *this;
    }

    sharedptr<TableInfo> m_info;

    type_vec_fields m_fields;
    type_vec_relationships m_relationships;

    typedef std::list< LayoutInfo > type_layouts;
    type_layouts m_layouts;

    typedef std::map< Glib::ustring, sharedptr<Report> > type_reports; //map of report names to reports
    type_reports m_reports;

    typedef std::map< Glib::ustring, sharedptr<PrintLayout> > type_print_layouts; //map of print layout names to print layouts
    type_print_layouts m_print_layouts;

    //Example data, used when creating a database from an example.
    type_example_rows m_example_rows;

    //Per-session, not saved in document:
    typedef std::map<Glib::ustring, Gnome::Gda::Value> type_map_layout_primarykeys;
    type_map_layout_primarykeys m_map_current_record; //The record last viewed in each layout.
    Glib::ustring m_layout_current;
    FoundSet m_foundset_current;

    float m_overviewx, m_overviewy;
  };

  DocumentTableInfo& get_table_info_with_add(const Glib::ustring& table_name);

  typedef std::map<Glib::ustring, DocumentTableInfo> type_tables;
  type_tables m_tables;


  //User groups:
  typedef std::map<Glib::ustring, GroupInfo> type_map_groups;
  type_map_groups m_groups;

  Glib::ustring m_database_title;
  Glib::ustring m_translation_original_locale;
  std::vector<Glib::ustring> m_translation_available_locales; //Just a cache, based on other data.

  typedef std::map<Glib::ustring, Glib::ustring> type_map_library_scripts;
  type_map_library_scripts m_map_library_scripts;

  Glib::ustring m_startup_script;

  bool m_block_cache_update; //For efficiency.
  bool m_block_modified_set;
  bool m_allow_auto_save;
  bool m_is_example;
  bool m_is_backup;
  guint m_document_format_version;

  bool m_opened_from_browse;

  Glib::ustring m_active_layout_platform; //empty (means normal), or "maemo".
};

} //namespace Glom

#endif // GLOM_DOCUMENT_H
