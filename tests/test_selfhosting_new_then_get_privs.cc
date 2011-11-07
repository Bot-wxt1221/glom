/* Glom
 *
 * Copyright (C) 2010 Openismus GmbH
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
71 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "tests/test_selfhosting_utils.h"
#include <libglom/init.h>
#include <libglom/privs.h>
#include <iostream>
#include <cstdlib> //For EXIT_SUCCESS and EXIT_FAILURE

static bool test(Glom::Document::HostingMode hosting_mode)
{
  Glom::Document document;
  const bool recreated = 
    test_create_and_selfhost_from_example("example_smallbusiness.glom", document, hosting_mode);
  if(!recreated)
  {
    std::cerr << "Recreation failed." << std::endl;
    return false;
  }
  
  const Glom::Privs::type_vec_strings groups = Glom::Privs::get_database_groups();
  if(groups.empty())
  {
    std::cerr << "Failure: groups was empty." << std::endl;
    return false;
  }

  for(Glom::Privs::type_vec_strings::const_iterator iter = groups.begin(); iter != groups.end(); ++iter)
  {
    const Glib::ustring group_name = *iter;
    if(group_name.empty())
    {
      std::cerr << "Failure: group_name was empty." << std::endl;
      return false;
    }

    const Glom::Privs::type_vec_strings users = Glom::Privs::get_database_users(group_name);
    for(Glom::Privs::type_vec_strings::const_iterator iter = users.begin(); iter != users.end(); ++iter)
    {
      const Glib::ustring user_name = *iter;
      if(user_name.empty())
      {
        std::cerr << "Failure: user_name was empty." << std::endl;
        return false;
      }

      std::cout << "group: " << group_name << ", has user: " << user_name << std::endl;
    }
  }

  test_selfhosting_cleanup();
 
  return true; 
}

int main()
{
  Glom::libglom_init();
  
  if(!test(Glom::Document::HOSTING_MODE_POSTGRES_SELF))
  {
    std::cerr << "Failed with PostgreSQL" << std::endl;
    test_selfhosting_cleanup();
    return EXIT_FAILURE;
  }
  
  /* SQLite does not have this feature:
  if(!test(Glom::Document::HOSTING_MODE_SQLITE))
  {
    std::cerr << "Failed with SQLite" << std::endl;
    test_selfhosting_cleanup();
    return EXIT_FAILURE;
  }
  */

  Glom::libglom_deinit();

  return EXIT_SUCCESS;
}
