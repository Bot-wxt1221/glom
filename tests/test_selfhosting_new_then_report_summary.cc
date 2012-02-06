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
#include <libglom/report_builder.h>
#include <glib.h> //For g_assert()
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

  const Glom::sharedptr<const Glom::Report> report = 
    document.get_report("invoices", "by_customer");
  if(!report)
  {
    std::cerr << "The report could not be found." << std::endl;
    return false;
  }

  Glom::FoundSet found_set; //TODO: Test a where clause.
  found_set.m_table_name = "invoices";

  const Glib::ustring locale = "";  /* original locale */
  Glom::ReportBuilder report_builder(locale);
  report_builder.set_document(&document);
  const Glib::ustring html = 
    report_builder.report_build(found_set, report);

  if(html.empty())
  {
    std::cerr << "Failed: html was empty." << std::endl;
    return false;
  }

  if(html.find("Yodda Yossarian") == std::string::npos)
  {
    std::cerr << "Failed: html did not contain the expected text." << std::endl;
    return false;
  }

  if(html.find("90.47") == std::string::npos)
  {
    std::cerr << "Failed: html did not contain the expected summary number." << std::endl;
    return false;
  }

  test_selfhosting_cleanup();
    
  return true;
}

int main()
{
  Glom::libglom_init();

  //Make sure that we use an en locale,
  //so we can test for the expected numeric output;
  setlocale(LC_ALL, "en_US.UTF-8");


  if(!test(Glom::Document::HOSTING_MODE_POSTGRES_SELF))
  {
    std::cerr << "Failed with PostgreSQL" << std::endl;
    test_selfhosting_cleanup();
    return EXIT_FAILURE;
  }
  
  if(!test(Glom::Document::HOSTING_MODE_SQLITE))
  {
    std::cerr << "Failed with SQLite" << std::endl;
    test_selfhosting_cleanup();
    return EXIT_FAILURE;
  }

  Glom::libglom_deinit();

  return EXIT_SUCCESS;
}
