/* Glom
 *
 * Copyright (C) 2012 Openismus GmbH
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

// For instance:
// glom_export_po /opt/gnome30/share/doc/glom/examples/example_music_collection.glom --output="/home/someone/something.po"

#include "config.h"

#include <libglom/init.h>
#include <libglom/translations_po.h>
#include <giomm/file.h>
#include <glibmm/optioncontext.h>
#include <glibmm/convert.h>
#include <glibmm/miscutils.h>
#include <iostream>

#include <glibmm/i18n.h>

class GlomCreateOptionGroup : public Glib::OptionGroup
{
public:
  GlomCreateOptionGroup();

  //These instances should live as long as the OptionGroup to which they are added,
  //and as long as the OptionContext to which those OptionGroups are added.
  std::string m_arg_filepath_output;
  Glib::ustring m_arg_locale_id;
  bool m_arg_version;
};

GlomCreateOptionGroup::GlomCreateOptionGroup()
: Glib::OptionGroup("glom_export_po_all", _("Glom options"), _("Command-line options")),
  m_arg_version(false)
{
  Glib::OptionEntry entry; 
  entry.set_long_name("output-path");
  entry.set_short_name('o');
  entry.set_description(_("The directory path at which to save the created .po files, such as /home/someuser/po_files/ ."));
  add_entry_filename(entry, m_arg_filepath_output);

  entry.set_long_name("version");
  entry.set_short_name('V');
  entry.set_description(_("The version of this application."));
  add_entry(entry, m_arg_version);
}

int main(int argc, char* argv[])
{
  Glom::libglom_init();
  
  Glib::OptionContext context;
  GlomCreateOptionGroup group;
  context.set_main_group(group);

  try
  {
    context.parse(argc, argv);
  }
  catch(const Glib::OptionError& ex)
  {
      std::cout << _("Error while parsing command-line options: ") << std::endl << ex.what() << std::endl;
      std::cout << _("Use --help to see a list of available command-line options.") << std::endl;
      return 0;
  }
  catch(const Glib::Error& ex)
  {
    std::cout << "Error: " << ex.what() << std::endl;
    return 0;
  }

  if(group.m_arg_version)
  {
    std::cout << PACKAGE_STRING << std::endl;
    return 0;
  }

  // Get a URI for a glom file:
  Glib::ustring input_uri;

  // The GOption documentation says that options without names will be returned to the application as "rest arguments".
  // I guess this means they will be left in the argv. Murray.
  if(input_uri.empty() && (argc > 1))
  {
     const char* pch = argv[1];
     if(pch)
       input_uri = pch;
  }

  if(input_uri.empty())
  {
    std::cerr << "Please specify a glom file." << std::endl;
    std::cerr << std::endl << context.get_help() << std::endl;
    return EXIT_FAILURE;
  }

  //Get a URI (file://something) from the filepath:
  Glib::RefPtr<Gio::File> file_input = Gio::File::create_for_commandline_arg(input_uri);

  //Make sure it is really a URI:
  input_uri = file_input->get_uri();

  if(!file_input->query_exists())
  {
    std::cerr << _("The Glom file does not exist.") << std::endl;
    std::cerr << "uri: " << input_uri << std::endl;

    std::cerr << std::endl << context.get_help() << std::endl;
    return EXIT_FAILURE;
  }

  Gio::FileType file_type = file_input->query_file_type();
  if(file_type == Gio::FILE_TYPE_DIRECTORY)
  {
    std::cerr << _("The Glom file path is a directory instead of a file.") << std::endl;
    std::cerr << std::endl << context.get_help() << std::endl;
    return EXIT_FAILURE;
  }

  //Check the output path: 
  if(group.m_arg_filepath_output.empty())
  {
    std::cerr << _("Please specify an output path.") << std::endl;
    std::cerr << std::endl << context.get_help() << std::endl;
    return EXIT_FAILURE;
  }

  //Get a URI (file://something) from the filepath:
  Glib::RefPtr<Gio::File> file_output = Gio::File::create_for_commandline_arg(group.m_arg_filepath_output);
  
  //Create the directory, if necessary:
  if(!(file_output->query_exists()))
  {
    if(!(file_output->make_directory_with_parents()))
    {
      std::cerr << _("The ouput directory could not be created.") << std::endl;
      return EXIT_FAILURE;
    }
  }

  file_type = file_output->query_file_type();
  if(file_type != Gio::FILE_TYPE_DIRECTORY)
  {
    std::cerr << _("Glom: The output file path is not a directory.") << std::endl;
    std::cerr << std::endl << context.get_help() << std::endl;
    return EXIT_FAILURE;
  }


  // Load the document:
  Glom::Document document;
  document.set_file_uri(input_uri);
  int failure_code = 0;
  const bool test = document.load(failure_code);
  //std::cout << "Document load result=" << test << std::endl;

  if(!test)
  {
    std::cerr << "Document::load() failed with failure_code=" << failure_code << std::endl;
    return EXIT_FAILURE;
  }

  const std::vector<Glib::ustring> locales = document.get_translation_available_locales();
  if(locales.empty())
  {
    std::cerr << "The Glom document has no translations." << std::endl;
    return EXIT_FAILURE;
  }

  const Glib::ustring original_locale_id = document.get_translation_original_locale();
  for(std::vector<Glib::ustring>::const_iterator iter = locales.begin();
    iter != locales.end(); ++iter)
  {
    const Glib::ustring locale_id = *iter;
    if(locale_id == original_locale_id)
      continue;

    const Glib::RefPtr<const Gio::File> file_output_full = file_output->get_child(locale_id + ".po");
    const Glib::ustring output_uri = file_output_full->get_uri();

    const bool succeeded = 
      Glom::write_translations_to_po_file(&document, output_uri, group.m_arg_locale_id);
    if(!succeeded)
    {
      std::cerr << "Po file creation failed." << std::endl;
      return EXIT_FAILURE;
    }

    std::cout << "Po file created at: " << output_uri << std::endl;
  }

  Glom::libglom_deinit();

  return EXIT_SUCCESS;
}
