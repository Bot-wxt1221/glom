/* Glom
 *
 * Copyright (C) 2001-2005 Murray Cumming
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

#include <libglom/python_embed/py_glom_relatedrecord.h>
#include <libglom/python_embed/py_glom_record.h>
#include <libglom/python_embed/pygdavalue_conversions.h> //For pygda_value_as_pyobject().
#include <libglom/connectionpool.h>
#include <libglom/data_structure/glomconversions.h>

#include <libglom/data_structure/field.h>
#include <glibmm/ustring.h>

//#include "glom/application.h"

namespace Glom
{

PyGlomRelatedRecord::PyGlomRelatedRecord()
{
}

PyGlomRelatedRecord::~PyGlomRelatedRecord()
{
}


static void RelatedRecord_HandlePythonError()
{
  if(PyErr_Occurred())
    PyErr_Print();
}

long PyGlomRelatedRecord::len() const
{
  return m_map_field_values.size();
}

boost::python::object PyGlomRelatedRecord::getitem(const boost::python::object& cppitem)
{
  const std::string field_name = boost::python::extract<std::string>(cppitem);

  PyGlomRelatedRecord::type_map_field_values::const_iterator iterFind = m_map_field_values.find(field_name);
  if(iterFind != m_map_field_values.end())
  {
    //If the value has already been stored, then just return it again:
    return glom_pygda_value_as_boost_pyobject(iterFind->second);
  }
  
  const Glib::ustring related_table = m_relationship->get_to_table();

  //Check whether the field exists in the table.
  //TODO_Performance: Do this without the useless Field information?
  sharedptr<const Field> field = m_document->get_field(m_relationship->get_to_table(), field_name);
  if(!field)
  {
    g_warning("PyGlomRelatedRecord::setitem(): field %s not found in table %s", field_name.c_str(), m_relationship->get_to_table().c_str());
    PyErr_SetString(PyExc_IndexError, "field not found");
    return boost::python::object();
  }
  else
  {
    //Try to get the value from the database:
    //const Glib::ustring parent_key_name;
#ifdef GLIBMM_EXCEPTIONS_ENABLED
    sharedptr<SharedConnection> sharedconnection = ConnectionPool::get_instance()->connect();
#else
    std::auto_ptr<ExceptionConnection> conn_error;
    sharedptr<SharedConnection> sharedconnection = ConnectionPool::get_instance()->connect(conn_error);
    // Ignore error, sharedconnection presence is checked below
#endif
    if(!sharedconnection)
    {
      PyErr_SetString(PyExc_IndexError, "connection not found");
      return boost::python::object();
    }
  
    Glib::RefPtr<Gnome::Gda::Connection> gda_connection = sharedconnection->get_gda_connection();

    const Glib::ustring related_key_name = m_relationship->get_to_field();

    //Do not try to get a value based on a null key value:
    if(Conversions::value_is_empty(m_from_key_value))
    {
      PyErr_SetString(PyExc_IndexError, "connection not found");
      return boost::python::object();
    }

    //Get the single value from the related records:
    Glib::RefPtr<Gnome::Gda::SqlBuilder> builder =
      Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_SELECT);
    builder->select_add_field(field_name, related_table);
    builder->select_add_target(related_table);
    builder->set_where(
      builder->add_cond(Gnome::Gda::SQL_OPERATOR_TYPE_EQ,
        builder->add_id(related_key_name), //TODO: It would nice to specify the table name here too.
        builder->add_expr(m_from_key_value)));
        
    /* TODO: Fix linking problems
    const App_Glom* app = App_Glom::get_application();
    if(app && app->get_show_sql_debug())
    {
      try
      {
        std::cout << "Debug: PyGlomRelatedRecord::setitem()():  " << sql_query << std::endl;
      }
      catch(const Glib::Exception& ex)
      {
        std::cout << "Debug: query string could not be converted to std::cout: " << ex.what() << std::endl;
      }
    }*/
    
    // TODO: Does this behave well if this throws an exception?
    Glib::RefPtr<Gnome::Gda::DataModel> datamodel = gda_connection->statement_execute_select_builder(builder);
    if(datamodel && datamodel->get_n_rows())
    {
      const Gnome::Gda::Value value = datamodel->get_value_at(0, 0);
      //g_warning("PyGlomRelatedRecord::setitem()(): value from datamodel = %s", value.to_string().c_str());

      //Cache it, in case it's asked-for again.
      m_map_field_values[field_name] = value;
      return glom_pygda_value_as_boost_pyobject(value);
    }
    else if(!datamodel)
    {
      g_warning("PyGlomRelatedRecord::setitem()(): The datamodel was null.");
      ConnectionPool::handle_error_cerr_only();
      RelatedRecord_HandlePythonError();
    }
    else
    {
      g_warning("PyGlomRelatedRecord::setitem()(): No related records exist yet for relationship %s.",  m_relationship->get_name().c_str());
    }
  }

  g_warning("PyGlomRelatedRecord::setitem()(): return null.");
  return boost::python::object();
}

boost::python::object PyGlomRelatedRecord::generic_aggregate(const std::string& field_name, const std::string& aggregate) const
{
  const Glib::ustring related_table = m_relationship->get_to_table();

  //Check whether the field exists in the table.
  //TODO_Performance: Do this without the useless Field information?
  sharedptr<Field> field = m_document->get_field(m_relationship->get_to_table(), field_name);
  if(!field)
  {
    g_warning("RelatedRecord_sum: field %s not found in table %s", field_name.c_str(), m_relationship->get_to_table().c_str());
    return boost::python::object();
  }

  //Try to get the value from the database:
  //const Glib::ustring parent_key_name;
#ifdef GLIBMM_EXCEPTIONS_ENABLED
  sharedptr<SharedConnection> sharedconnection = ConnectionPool::get_instance()->connect();
#else
  std::auto_ptr<ExceptionConnection> conn_error;
  sharedptr<SharedConnection> sharedconnection = ConnectionPool::get_instance()->connect(conn_error);
  // Ignore error, sharedconnection presence is checked below
#endif
  if(!sharedconnection)
  {
    g_warning("RelatedRecord_sum: no connection.");
    return boost::python::object();
  }

  Glib::RefPtr<Gnome::Gda::Connection> gda_connection = sharedconnection->get_gda_connection();

  const Glib::ustring related_key_name = m_relationship->get_to_field();

  //Do not try to get a value based on a null key value:
  if(Conversions::value_is_empty(m_from_key_value))
  {
    return boost::python::object();
  }

  //Get the aggregate value from the related records:
  Glib::RefPtr<Gnome::Gda::SqlBuilder> builder =
    Gnome::Gda::SqlBuilder::create(Gnome::Gda::SQL_STATEMENT_SELECT);
  builder->add_function(aggregate, builder->add_id(field_name)); //TODO: It would be nice to specify the table here too.
  builder->select_add_target(related_table);
  builder->set_where(
    builder->add_cond(Gnome::Gda::SQL_OPERATOR_TYPE_EQ,
      builder->add_id(related_key_name), //TODO: It would nice to specify the table name here too.
      builder->add_expr(m_from_key_value)));

 
  //std::cout << "PyGlomRelatedRecord: Executing:  " << sql_query << std::endl;
  Glib::RefPtr<Gnome::Gda::DataModel> datamodel = gda_connection->statement_execute_select_builder(builder);

  // Ignore the error: The case that the command execution didn't return
  // a datamodel is handled below.
  if(datamodel && datamodel->get_n_rows())
  {
    Gnome::Gda::Value value = datamodel->get_value_at(0, 0);
    //g_warning("RelatedRecord_generic_aggregate(): value from datamodel = %s", value.to_string().c_str());

    //Cache it, in case it's asked-for again.
    m_map_field_values[field_name] = value;
    return glom_pygda_value_as_boost_pyobject(value);
  }
  else if(!datamodel)
  {
    g_warning("RelatedRecord_generic_aggregate(): The datamodel was null.");
    ConnectionPool::handle_error_cerr_only();
    RelatedRecord_HandlePythonError();
  }
  else
  {
    g_warning("RelatedRecord_generic_aggregate(): No related records exist yet for relationship %s.",  m_relationship->get_name().c_str());
  }

  return boost::python::object();
}

boost::python::object PyGlomRelatedRecord::sum(const std::string& field_name) const
{
  return generic_aggregate(field_name, "sum");
}

boost::python::object PyGlomRelatedRecord::count(const std::string& field_name) const
{
  return generic_aggregate(field_name, "count");
}

boost::python::object PyGlomRelatedRecord::min(const std::string& field_name) const
{
  return generic_aggregate(field_name, "min");
}

boost::python::object PyGlomRelatedRecord::max(const std::string& field_name) const
{
  return generic_aggregate(field_name, "max");
}

void PyGlomRelatedRecord_SetRelationship(PyGlomRelatedRecord* self, const sharedptr<const Relationship>& relationship, const Gnome::Gda::Value& from_key_value,  Document* document)
{
  self->m_relationship = relationship;

  self->m_from_key_value = from_key_value;

  self->m_document = document;
}

} //namespace Glom
