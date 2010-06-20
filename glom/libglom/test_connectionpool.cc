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
#include <libglom/connectionpool.h>

#define GLOM_ENABLE_POSTGRESQL

#ifdef GLOM_ENABLE_POSTGRESQL
#include <libglom/connectionpool_backends/postgres_central.h>
#else
#include <libglom/connectionpool_backends/sqlite.h>
#endif //#GLOM_ENABLE_POSTGRESQL


int
main()
{
  Gnome::Gda::init();

  Glib::RefPtr<Gnome::Gda::Connection> gdaconnection;

  {
    //Set the connection details:
    Glom::ConnectionPool* connection_pool = Glom::ConnectionPool::get_instance();
    if(connection_pool)
    {
      //Set the connection details in the ConnectionPool singleton.
      //The ConnectionPool will now use these every time it tries to connect.

      connection_pool->set_database("glom_musiccollection217");
      connection_pool->set_user("murrayc");
      connection_pool->set_password("murraycpw");

#ifdef GLOM_ENABLE_POSTGRESQL
      Glom::ConnectionPoolBackends::PostgresCentralHosted* backend = new Glom::ConnectionPoolBackends::PostgresCentralHosted;
      backend->set_host("localhost");
      backend->set_port(5433);
      backend->set_try_other_ports(false);
#else
      Glom::ConnectionPoolBackends::Sqlite* backend = new Glom::ConnectionPoolBackends::Sqlite;
#endif //GLOM_ENABLE_POSTGRESQL

      connection_pool->set_backend(std::auto_ptr<Glom::ConnectionPool::Backend>(backend));
      connection_pool->set_ready_to_connect(); //connect_to_server() will now attempt the connection-> Shared instances of m_Connection will also be usable.
    }

    //Connect:
    Glom::sharedptr<Glom::SharedConnection> connection;
    connection = Glom::ConnectionPool::get_and_connect();

    if(connection)
      std::cout << "Connected" << std::endl;
    else
      std::cout << "Connection failed." << std::endl;

    gdaconnection = connection->get_gda_connection();
    //Cleanup:
    Glom::ConnectionPool::delete_instance();
  }

  std::cout << "gdaconnection refcount=" << G_OBJECT(gdaconnection->gobj())->ref_count << std::endl;


  return 0;
}





