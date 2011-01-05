#include <glom/bakery/busy_cursor.h>
#include <gtkmm/main.h>
#include <gtk/gtk.h>

namespace Glom
{

//Intialize static member variable:
BusyCursor::type_map_cursors BusyCursor::m_map_cursors;

BusyCursor::BusyCursor(Gtk::Window& window, Gdk::CursorType cursor_type)
: m_Cursor( Gdk::Cursor::create(cursor_type) ),
  m_pWindow(&window) //If this is a nested cursor then remember the previously-set cursor, so we can restore it.
{
  init();
}

BusyCursor::BusyCursor(Gtk::Window* window, Gdk::CursorType cursor_type)
: m_Cursor( Gdk::Cursor::create(cursor_type) ),
  m_pWindow(window) //If this is a nested cursor then remember the previously-set cursor, so we can restore it.
{
  if(m_pWindow)
    init();
}

void BusyCursor::init()
{
  if(!m_pWindow)
    return;

  m_refWindow = m_pWindow->get_window();
  if(!m_refWindow)
    return;

  type_map_cursors::iterator iter = m_map_cursors.find(m_pWindow);
  if(iter != m_map_cursors.end())
  {
    m_old_cursor = iter->second; //Remember the existing cursor.
  }

  m_map_cursors[m_pWindow] = m_Cursor; //Let a further nested cursor know about the new cursor in use.


  //Change the cursor:
  if(m_refWindow)
    m_refWindow->set_cursor(m_Cursor);

  force_gui_update();
}

BusyCursor::~BusyCursor()
{
  //Restore the old cursor:
  if(m_old_cursor)
  {
    if(m_refWindow)
      m_refWindow->set_cursor(m_old_cursor);
  }
  else
  {
    if(m_refWindow)
      m_refWindow->set_cursor();

    type_map_cursors::iterator iter = m_map_cursors.find(m_pWindow);
    if(iter != m_map_cursors.end())
      m_map_cursors.erase(iter);
  }

  force_gui_update();
}


void BusyCursor::force_gui_update()
{
  if(m_refWindow)
  {
    //Force the GUI to update:
    while(Gtk::Main::events_pending())
      Gtk::Main::iteration();
  }
}


} //namespace Glom
