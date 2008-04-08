/* Glom
 *
 * Copyright (C) 2008 Murray Cumming
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
 
#include <libgdamm/init.h>
#include <glom/libglom/connectionpool.h>


int
main(int argc, char* argv[])
{
  Gnome::Gda::init("test", "0.1", argc, argv);

  //Set the connection details:
  Glom::ConnectionPool* connection_pool = Glom::ConnectionPool::get_instance();
  if(connection_pool)
  {
    //Set the connection details in the ConnectionPool singleton.
    //The ConnectionPool will now use these every time it tries to connect.

    connection_pool->set_database("glom_musiccollection217");
    connection_pool->set_host("localhost");
    connection_pool->set_user("murrayc");
    connection_pool->set_password("murraycpw");

    connection_pool->set_port(5433);
    connection_pool->set_try_other_ports(false);

    connection_pool->set_ready_to_connect(); //Box_DB::connect_to_server() will now attempt the connection-> Shared instances of m_Connection will also be usable.
  }

  //Connect:
  Glom::sharedptr<Glom::SharedConnection> connection;
#ifdef GLIBMM_EXCEPTIONS_ENABLED
  connection = Glom::ConnectionPool::get_and_connect();
#else
  std::auto_ptr<ExceptionConnection> error;
  connection = Glom::ConnectionPool::get_and_connect(error);
#endif

  if(connection)
    std::cout << "Connected" << std::endl;
  else
    std::cout << "Connection failed." << std::endl;

  //Cleanup:
  Glom::ConnectionPool::delete_instance();

  return 0;
}





