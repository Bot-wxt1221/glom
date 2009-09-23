#include <glom/import_csv/csv_parser.h>
#include <tests/import/utils.h>
//#include <glibmm/regex.h>
#include <gtkmm.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

namespace {

typedef std::vector<std::string> type_encodings;

guint& get_line_scanned_count_instance()
{
  static guint line_scanned_count = 0;
  return line_scanned_count;
}

guint& get_encoding_error_count_instance()
{
  static guint encoding_error_count = 0;
  return encoding_error_count;
}

void on_line_scanned()
{
  ++(get_line_scanned_count_instance());
}

void on_encoding_error()
{
  ++(get_encoding_error_count_instance());
}

void reset_signal_counts()
{
  get_line_scanned_count_instance() = 0;
  get_encoding_error_count_instance() = 0;
}

void print_signal_counts()
{
  std::cout << "lines scanned: " << get_line_scanned_count_instance() << std::endl;
  std::cout << "encoding errors: " << get_encoding_error_count_instance() << std::endl;
}

} // namespace

// Testcases
int main(int argc, char* argv[])
{
  Gtk::Main gtk(argc, argv);

  Glom::CsvParser parser("UTF-8");
  parser.signal_line_scanned().connect(sigc::hide(sigc::hide(&on_line_scanned)));
  parser.signal_encoding_error().connect(sigc::ptr_fun(&on_encoding_error));

  bool result = true;
  std::stringstream report;

  // test_ignore_quoted_newlines
  {
    // 2 CSV lines, first one contains newlines inside quotes
    const char raw[] = "\"some\n quoted\r\n newlines\n\", \"token2\"\n\"token3\"\n";
    ImportTests::set_parser_contents(parser, raw, sizeof(raw));

    while(parser.on_idle_parse())
    {}

    bool passed = (2 == get_line_scanned_count_instance() &&
                   0 == get_encoding_error_count_instance());

    if(!ImportTests::check("test_ignore_quoted_newlines", passed, report))
      result = false;

    reset_signal_counts();
    parser.clear();
  }

  // test_ignore_empty_lines
  {
    // 5 CSV lines, but only 2 contain data
    const char raw[] = "token1\n\n\n\ntoken2, token3\n";
    ImportTests::set_parser_contents(parser, raw, sizeof(raw));

    while(parser.on_idle_parse())
    {}

    const bool passed = (2 == get_line_scanned_count_instance() &&
                   0 == get_encoding_error_count_instance());

    if(!ImportTests::check("test_ignore_empty_lines", passed, report))
      result = false;

    reset_signal_counts();
    parser.clear();
  }

  // test_wrong_encoding
  {
    const char* const encoding_arr[] = {"UTF-8", "UCS-2"};
    type_encodings encodings(encoding_arr, encoding_arr + G_N_ELEMENTS(encoding_arr));

    // An invalid Unicode sequence.
    const char raw[] = "\0xc0\0x00\n";
    ImportTests::set_parser_contents(parser, raw, sizeof(raw));

    for (type_encodings::const_iterator iter = encodings.begin();
         iter != encodings.end();
         ++iter)
    {
      #ifdef GLIBMM_EXCEPTIONS_ENABLED
      try
      {
        while(parser.on_idle_parse())
        {}

        parser.clear();
      }
      catch(const Glib::ConvertError& exception)
      {
        std::cout << exception.what() << std::endl;
      }
      #else
      while(parser.on_idle_parse())
      {}

      parser.clear();
      #endif

      parser.set_encoding((*iter).c_str());
    }


    const bool passed = (2 == get_encoding_error_count_instance() &&
                   0 == get_line_scanned_count_instance());

    if(!ImportTests::check("test_wrong_encoding", passed, report))
      result = false;

    reset_signal_counts();
    parser.clear();
  }

  // test_incomplete_chars
  {
    // An incomplete Unicode sequence.
    const char raw[] = "\0xc0\n";
    ImportTests::set_parser_contents(parser, raw, sizeof(raw));

    while(parser.on_idle_parse())
    {}

    const bool passed = (1 == get_encoding_error_count_instance() &&
                   0 == get_line_scanned_count_instance());

    if(!ImportTests::check("test_incomplete_chars", passed, report))
      result = false;

    reset_signal_counts();
    parser.clear();
  }

  if(!result)
    std::cout << report.rdbuf() << std::endl;

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}

