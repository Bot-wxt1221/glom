/* This is copied from gtkmm 2.15/16 so we can use new API without waiting 
 * for stable release of GTK+ 2.16. Life would be easier if GTK+ followed 
 * the GNOME release schedule.
 */
//#include <gtk/gtkcalendar.h>
#include <gtkmm/calendar.h>
#include <gtk/gtk.h>
#if GTK_CHECK_VERSION(2,14,0)
namespace GlomGtk
{
  typedef Gtk::Calendar Calendar;
}
#else


// -*- c++ -*-
// Generated by gtkmmproc -- DO NOT MODIFY!
#ifndef _GLOM_GTKMM_CALENDAR_H
#define _GLOM_GTKMM_CALENDAR_H


#include <glibmm.h>

/* $Id: calendar.hg,v 1.6 2006/02/25 12:38:11 murrayc Exp $ */

/* calendar.h
 * 
 * Copyright (C) 1998-2002 The gtkmm Development Team
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

#include <gtkmm/widget.h>
#include <glibmm/date.h>


#ifndef DOXYGEN_SHOULD_SKIP_THIS
typedef struct _GlomGtkCalendar GlomGtkCalendar;
typedef struct _GlomGtkCalendarClass GlomGtkCalendarClass;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace GlomGtk
{ class Calendar_Class; } // namespace GlomGtk
namespace GlomGtk
{


/** @addtogroup gtkmmEnums Enums and Flags */

/**
 * @ingroup gtkmmEnums
 * @par Bitwise operators:
 * <tt>%CalendarDisplayOptions operator|(CalendarDisplayOptions, CalendarDisplayOptions)</tt><br>
 * <tt>%CalendarDisplayOptions operator&(CalendarDisplayOptions, CalendarDisplayOptions)</tt><br>
 * <tt>%CalendarDisplayOptions operator^(CalendarDisplayOptions, CalendarDisplayOptions)</tt><br>
 * <tt>%CalendarDisplayOptions operator~(CalendarDisplayOptions)</tt><br>
 * <tt>%CalendarDisplayOptions& operator|=(CalendarDisplayOptions&, CalendarDisplayOptions)</tt><br>
 * <tt>%CalendarDisplayOptions& operator&=(CalendarDisplayOptions&, CalendarDisplayOptions)</tt><br>
 * <tt>%CalendarDisplayOptions& operator^=(CalendarDisplayOptions&, CalendarDisplayOptions)</tt><br>
 */
enum CalendarDisplayOptions
{
  CALENDAR_SHOW_HEADING = 1 << 0,
  CALENDAR_SHOW_DAY_NAMES = 1 << 1,
  CALENDAR_NO_MONTH_CHANGE = 1 << 2,
  CALENDAR_SHOW_WEEK_NUMBERS = 1 << 3,
  CALENDAR_WEEK_START_MONDAY = 1 << 4,
  CALENDAR_SHOW_DETAILS = 1 << 5
};

/** @ingroup gtkmmEnums */
inline CalendarDisplayOptions operator|(CalendarDisplayOptions lhs, CalendarDisplayOptions rhs)
  { return static_cast<CalendarDisplayOptions>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs)); }

/** @ingroup gtkmmEnums */
inline CalendarDisplayOptions operator&(CalendarDisplayOptions lhs, CalendarDisplayOptions rhs)
  { return static_cast<CalendarDisplayOptions>(static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs)); }

/** @ingroup gtkmmEnums */
inline CalendarDisplayOptions operator^(CalendarDisplayOptions lhs, CalendarDisplayOptions rhs)
  { return static_cast<CalendarDisplayOptions>(static_cast<unsigned>(lhs) ^ static_cast<unsigned>(rhs)); }

/** @ingroup gtkmmEnums */
inline CalendarDisplayOptions operator~(CalendarDisplayOptions flags)
  { return static_cast<CalendarDisplayOptions>(~static_cast<unsigned>(flags)); }

/** @ingroup gtkmmEnums */
inline CalendarDisplayOptions& operator|=(CalendarDisplayOptions& lhs, CalendarDisplayOptions rhs)
  { return (lhs = static_cast<CalendarDisplayOptions>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs))); }

/** @ingroup gtkmmEnums */
inline CalendarDisplayOptions& operator&=(CalendarDisplayOptions& lhs, CalendarDisplayOptions rhs)
  { return (lhs = static_cast<CalendarDisplayOptions>(static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs))); }

/** @ingroup gtkmmEnums */
inline CalendarDisplayOptions& operator^=(CalendarDisplayOptions& lhs, CalendarDisplayOptions rhs)
  { return (lhs = static_cast<CalendarDisplayOptions>(static_cast<unsigned>(lhs) ^ static_cast<unsigned>(rhs))); }

} // namespace GlomGtk



namespace GlomGtk
{


/** Display a calendar and/or allow the user to select a date.
 *
 * This is a widget that displays a calendar, one month at a time.
 *
 * The month and year currently displayed can be altered with
 * select_month().  The exact day can be selected from the displayed month
 * using select_day().
 *
 * The way in which the calendar itself is displayed can be altered using
 * set_display_options().
 *
 * The selected date can be retrieved from a GlomGtkCalendar using get_date().
 *
 * If performing many 'mark' operations, the calendar can be frozen to
 * prevent flicker, using freeze(), and 'thawed' again using thaw().
 *
 * @ingroup Widgets
 */

class Calendar : public Gtk::Widget
{
  public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  typedef Calendar CppObjectType;
  typedef Calendar_Class CppClassType;
  typedef GlomGtkCalendar BaseObjectType;
  typedef GlomGtkCalendarClass BaseClassType;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

  virtual ~Calendar();

#ifndef DOXYGEN_SHOULD_SKIP_THIS

private:
  friend class Calendar_Class;
  static CppClassType calendar_class_;

  // noncopyable
  Calendar(const Calendar&);
  Calendar& operator=(const Calendar&);

protected:
  explicit Calendar(const Glib::ConstructParams& construct_params);
  explicit Calendar(GlomGtkCalendar* castitem);

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  static GType get_type()      G_GNUC_CONST;
  static GType get_base_type() G_GNUC_CONST;
#endif

  ///Provides access to the underlying C GtkObject.
  GlomGtkCalendar*       gobj()       { return reinterpret_cast<GlomGtkCalendar*>(gobject_); }

  ///Provides access to the underlying C GtkObject.
  const GlomGtkCalendar* gobj() const { return reinterpret_cast<GlomGtkCalendar*>(gobject_); }


public:
  //C++ methods used to invoke GTK+ virtual functions:
#ifdef GLIBMM_VFUNCS_ENABLED
#endif //GLIBMM_VFUNCS_ENABLED

protected:
  //GTK+ Virtual Functions (override these to change behaviour):
#ifdef GLIBMM_VFUNCS_ENABLED
#endif //GLIBMM_VFUNCS_ENABLED

  //Default Signal Handlers::
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
  virtual void on_month_changed();
  virtual void on_day_selected();
  virtual void on_day_selected_double_click();
  virtual void on_prev_month();
  virtual void on_next_month();
  virtual void on_prev_year();
  virtual void on_next_year();
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED


private:

public:
  Calendar();
  
  
  /** Shifts the calendar to a different month.
   * @param month A month number between 0 and 11.
   * @param year The year the month is in.
   * @return <tt>true</tt>, always.
   */
  int select_month(guint month, guint year);
  
  /** Selects a day from the current month.
   * @param day The day number between 1 and 31, or 0 to unselect 
   * the currently selected day.
   */
  void select_day(guint day);
  
  /** Places a visual marker on a particular day.
   * @param day The day number to mark between 1 and 31.
   * @return <tt>true</tt>, always.
   */
  int mark_day(guint day);
  
  /** Removes the visual marker from a particular day.
   * @param day The day number to unmark between 1 and 31.
   * @return <tt>true</tt>, always.
   */
  int unmark_day(guint day);
  
  /** Remove all visual markers.
   */
  void clear_marks();

  
#ifndef GTKMM_DISABLE_DEPRECATED

  /** Sets display options (whether to display the heading and the month headings).
   * 
   * Deprecated: 2.4: Use set_display_options() instead
   * @param flags The display options to set.
   */
  void display_options(CalendarDisplayOptions flags);
#endif // GTKMM_DISABLE_DEPRECATED


  /** Sets display options (whether to display the heading and the month  
   * headings).
   * 
   * @newin2p4
   * @param flags The display options to set.
   */
  void set_display_options(CalendarDisplayOptions flags);
  
  /** Return value: the display options.
   * @return The display options.
   * 
   * @newin2p4.
   */
  CalendarDisplayOptions get_display_options() const;

  
  /** Obtains the selected date from a GlomGtk::Calendar.
   * @param year Location to store the year number, or <tt>0</tt>.
   * @param month Location to store the month number (between 0 and 11), or <tt>0</tt>.
   * @param day Location to store the day number (between 1 and 31), or <tt>0</tt>.
   */
  void get_date(guint& year, guint& month, guint& day) const;
  // Additional method not in GTK (#350584)
  /** Get the date as Glib::Date
   *
   * @param date Referenz to set the date
   */
  void get_date(Glib::Date& date) const;
  

  /** For instance,
   * @code
   * Glib::ustring on_calendar_details(guint year, guint month, guint day);
   * @endcode
   *
   * @param year The year for which details are needed.
   * @param month The month for which details are needed.
   * @param day The day of @month for which details are needed.
   * @result Pango markup with details for the specified day, or an empty string.
   *
   * This callback function provides Pango markup with detail information for the
   * specified day. Examples for such details are holidays or appointments. The
   * function should return an empty string when no information is available.
   *
   * @newin2p16 
   */
  typedef sigc::slot<Glib::ustring, guint, guint, guint> SlotDetails;


  /** Installs a function which provides Pango markup with detail information
   * for each day. Examples for such details are holidays or appointments. That
   * information is shown below each day when show-details is set.
   * A tooltip containing full detail information is provided if the entire
   * text could not fit into the details area, or if the show-details property
   * is not set.
   *
   * The size of the details area can be restricted by setting the
   * detail-width-chars and detail-height-rows properties.
   *
   * @param slot A callback function providing details for each day.
   *
   * @newin2p16
   */
  void set_detail_func(const SlotDetails& slot);
  
  
  void set_detail_width_chars(int chars);
  
  void set_detail_height_rows(int rows);
 
  
  int get_detail_width_chars() const;
  
  int get_detail_height_rows() const;

  
  /** Does nothing. Previously locked the display of the calendar until
   * it was thawed with thaw().
   * 
   * Deprecated: 2.8:
   */
  void freeze();
  
  /** Does nothing. Previously defrosted a calendar; all the changes made
   * since the last freeze() were displayed.
   * 
   * Deprecated: 2.8:
   */
  void thaw();

  
  /**
   * @par Prototype:
   * <tt>void on_my_%month_changed()</tt>
   */

  Glib::SignalProxy0< void > signal_month_changed();

  
  /**
   * @par Prototype:
   * <tt>void on_my_%day_selected()</tt>
   */

  Glib::SignalProxy0< void > signal_day_selected();

  
  /**
   * @par Prototype:
   * <tt>void on_my_%day_selected_double_click()</tt>
   */

  Glib::SignalProxy0< void > signal_day_selected_double_click();

  
  /**
   * @par Prototype:
   * <tt>void on_my_%prev_month()</tt>
   */

  Glib::SignalProxy0< void > signal_prev_month();

  
  /**
   * @par Prototype:
   * <tt>void on_my_%next_month()</tt>
   */

  Glib::SignalProxy0< void > signal_next_month();

  
  /**
   * @par Prototype:
   * <tt>void on_my_%prev_year()</tt>
   */

  Glib::SignalProxy0< void > signal_prev_year();

  
  /**
   * @par Prototype:
   * <tt>void on_my_%next_year()</tt>
   */

  Glib::SignalProxy0< void > signal_next_year();


  #ifdef GLIBMM_PROPERTIES_ENABLED
/** The selected year.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<int> property_year() ;
#endif //#GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
/** The selected year.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<int> property_year() const;
#endif //#GLIBMM_PROPERTIES_ENABLED

  #ifdef GLIBMM_PROPERTIES_ENABLED
/** The selected month (as a number between 0 and 11).
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<int> property_month() ;
#endif //#GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
/** The selected month (as a number between 0 and 11).
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<int> property_month() const;
#endif //#GLIBMM_PROPERTIES_ENABLED

  #ifdef GLIBMM_PROPERTIES_ENABLED
/** The selected day (as a number between 1 and 31, or 0 to unselect the currently selected day).
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<int> property_day() ;
#endif //#GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
/** The selected day (as a number between 1 and 31, or 0 to unselect the currently selected day).
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<int> property_day() const;
#endif //#GLIBMM_PROPERTIES_ENABLED

  #ifdef GLIBMM_PROPERTIES_ENABLED
/** If TRUE
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<bool> property_show_heading() ;
#endif //#GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
/** If TRUE
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<bool> property_show_heading() const;
#endif //#GLIBMM_PROPERTIES_ENABLED

  #ifdef GLIBMM_PROPERTIES_ENABLED
/** If TRUE
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<bool> property_show_day_names() ;
#endif //#GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
/** If TRUE
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<bool> property_show_day_names() const;
#endif //#GLIBMM_PROPERTIES_ENABLED

  #ifdef GLIBMM_PROPERTIES_ENABLED
/** If TRUE
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<bool> property_no_month_change() ;
#endif //#GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
/** If TRUE
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<bool> property_no_month_change() const;
#endif //#GLIBMM_PROPERTIES_ENABLED

  #ifdef GLIBMM_PROPERTIES_ENABLED
/** If TRUE
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<bool> property_show_week_numbers() ;
#endif //#GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
/** If TRUE
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<bool> property_show_week_numbers() const;
#endif //#GLIBMM_PROPERTIES_ENABLED

  //gtkmmproc error: show-details : property defs lookup failed.;
  //gtkmmproc error: detail-width-chars : property defs lookup failed.;
  //gtkmmproc error: detail-height-chars : property defs lookup failed.;


};

} /* namespace GlomGtk */


namespace Glib
{
  /** A Glib::wrap() method for this object.
   * 
   * @param object The C instance.
   * @param take_copy False if the result should take ownership of the C instance. True if it should take a new copy or ref.
   * @result A C++ instance that wraps this C instance.
   *
   * @relates GlomGtk::Calendar
   */
  GlomGtk::Calendar* wrap(GlomGtkCalendar* object, bool take_copy = false);
} //namespace Glib


#endif /* _GLOM_GTKMM_CALENDAR_H */


#endif //GTK_CHECK_VERSION(2,16,0)
