#include <glom/libglom/init.h>
#include <glom/libglom/connectionpool.h>
#include <glom/python_embed/glom_python.h>
#include <libglom/data_structure/glomconversions.h>
#include <libglom/utils.h>
#include <boost/python.hpp>
#include <iostream>

static void on_startup_progress()
{
  std::cout << "Database startup progress" << std::endl;
}

static void on_cleanup_progress()
{
  std::cout << "Database cleanup progress" << std::endl;
}

void cleanup()
{
  Glom::ConnectionPool* connection_pool = Glom::ConnectionPool::get_instance();

  const bool stopped = connection_pool->cleanup( sigc::ptr_fun(&on_cleanup_progress) );
  g_assert(stopped);
}

int main()
{
  Glom::libglom_init(); //Also initializes python.

  //Connect to a Glom database
  //A sqlite-based one, to simplify this test.
  // Get a URI for a test file:
  Glib::ustring uri;

  try
  {
    const std::string path =
       Glib::build_filename(GLOM_DOCDIR_EXAMPLES_NOTINSTALLED,
         "sqlite", "test_sqlite_music", "test_sqlite_music.glom");
    uri = Glib::filename_to_uri(path);
  }
  catch(const Glib::ConvertError& ex)
  {
    std::cerr << G_STRFUNC << ": " << ex.what();
    return EXIT_FAILURE;
  }

  g_assert( Glom::Utils::file_exists(uri) );
  //std::cout << "URI=" << uri << std::endl;


  // Load the document:
  Glom::Document document;
  document.set_file_uri(uri);
  int failure_code = 0;
  const bool test = document.load(failure_code);
  //std::cout << "Document load result=" << test << std::endl;

  if(!test)
  {
    std::cerr << "Document::load() failed with failure_code=" << failure_code << std::endl;
    return EXIT_FAILURE;
  }

  g_assert(!document.get_is_example_file());;

  Glom::ConnectionPool* connection_pool = Glom::ConnectionPool::get_instance();
  connection_pool->setup_from_document(&document);

  //This is not really necessary for sqlite-based databases.
  const Glom::ConnectionPool::StartupErrors started =
    connection_pool->startup( sigc::ptr_fun(&on_startup_progress) );
  if(started != Glom::ConnectionPool::Backend::STARTUPERROR_NONE)
  {
    std::cerr << "connection_pool->startup(): result=" << started << std::endl;
  }
  g_assert(started == Glom::ConnectionPool::Backend::STARTUPERROR_NONE);

  Glom::sharedptr<Glom::SharedConnection> connection = connection_pool->connect();
  g_assert(connection);

  Glib::RefPtr<Gnome::Gda::Connection> gda_connection = connection->get_gda_connection();
  g_assert(connection->get_gda_connection());


  //Some silly python code just to exercise our PyGlomRecord API:
  const char* calculation = "connection = record.connection\nreturn connection.is_opened()";
  Glom::type_map_fields field_values;

  //TODO: Use this: const type_map_fields field_values = get_record_field_values_for_calculation(field_in_record.m_table_name, field_in_record.m_key, field_in_record.m_key_value);
  //    if(!field_values.empty())

  //Execute a python function:
  Gnome::Gda::Value value;
  Glib::ustring error_message;
  try
  {
    value = Glom::glom_evaluate_python_function_implementation(
      Glom::Field::TYPE_BOOLEAN, calculation, field_values,
      0 /* document */, "" /* table name */,
      Glom::sharedptr<Glom::Field>(), Gnome::Gda::Value(), // primary key details. Not used in this test.
      gda_connection,
      error_message);
  }
  catch(const std::exception& ex)
  {
    std::cerr << "Exception: " << ex.what() << std::endl;
    return EXIT_FAILURE;
  }
  catch(const boost::python::error_already_set& ex)
  {
    std::cerr << "Exception: boost::python::error_already_set" << std::endl;
    return EXIT_FAILURE;
  }

  //std::cout << "type=" << g_type_name(value.get_value_type()) << std::endl;

  //Check that there was no python error:
  if(!error_message.empty())
  {
    std::cerr << "Python error: " << error_message << std::endl;
    return EXIT_FAILURE;
  }

  //Check that the return value is of the expected type:
  g_assert(value.get_value_type() == G_TYPE_BOOLEAN);

  //Check that the return value is of the expected value:
  const double boolval = value.get_boolean();
  g_assert(boolval == true);

  //std::cout << "value=" << value.to_string() << std::endl;

  cleanup();

  return EXIT_SUCCESS;
}
