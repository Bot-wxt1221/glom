#include <glom/import_csv/csv_parser.h>
#include <tests/import/utils.h>
//#include <glibmm/regex.h>
#include <gtkmm.h>
#include <iostream>
#include <cstdlib>

namespace
{

typedef std::vector<std::string> type_tokens;

type_tokens& get_tokens_instance()
{
  static type_tokens tokens;
  return tokens;
}


void on_line_scanned(const Glib::ustring& line, guint line_number);

void print_tokens()
{
  for(type_tokens::const_iterator iter = get_tokens_instance().begin();
       iter != get_tokens_instance().end();
       ++iter)
  {
    std::cout << " [" << *iter << "] ";
  }

  std::cout << std::endl;
}

bool check_tokens(const std::string& regex)
{
  Glib::RefPtr<Glib::Regex> check;
  
  #ifdef GLIBMM_EXCEPTIONS_ENABLED
  try
  {
    check = Glib::Regex::create(regex);
  }
  catch(const Glib::Error& ex)
  {
    std::cerr << "Glib::Regex::create() failed: " << ex.what() << std::endl;
    return false;
  } 
  #else
  std::auto_ptr<Glib::Error> ex;
  check = Glib::Regex::create(regex, static_cast<Glib::RegexCompileFlags>(0), static_cast<Glib::RegexMatchFlags>(0), ex);
  if(ex.get())
  {
    std::cerr << "Glib::Regex::create() failed: " << ex->what() << std::endl;
    return false;
  }
  #endif
 
  if(!check)
    return false;
     
  for(type_tokens::const_iterator iter = get_tokens_instance().begin();
       iter != get_tokens_instance().end();
       ++iter)
  {
    if(!check->match(*iter))
      return false;
  }

  return true;
}

void on_line_scanned(const Glib::ustring& line, guint /*line_number*/)
{
  Glib::ustring field;
  Glib::ustring::const_iterator line_iter(line.begin());

  while(line_iter != line.end())
  {
    line_iter = Glom::CsvParser::advance_field(line_iter, line.end(), field);
    get_tokens_instance().push_back(field);

    // Manually have to skip separators.
    if(',' == *line_iter)
    {
      get_tokens_instance().push_back(",");
      ++line_iter;
    }
  }
}

} // namespace

// Testcases
int main(int argc, char* argv[])
{
  Gtk::Main gtk(argc, argv);

  Glom::CsvParser parser("UTF-8");
  parser.signal_line_scanned().connect(sigc::ptr_fun(&on_line_scanned));

  bool result = true;
  std::stringstream report;

  // test_dquoted_string
  {
    const char raw_line[] = "\"a \"\"quoted\"\" token\",\"sans quotes\"\n";
    ImportTests::set_parser_contents(parser, raw_line, sizeof(raw_line));

    parser.set_file_and_start_parsing("./tests/import/data/dquoted_string.csv");

    while(parser.on_idle_parse())
    {}

    bool passed = check_tokens("^(a \"quoted\" token|,|sans quotes)$");
    if(!ImportTests::check("test_dquoted_string", passed, report))
      result = false;

    get_tokens_instance().clear();
    parser.clear();
  }

  // test_skip_on_no_ending_newline
  {
    const char raw_line[] = "\"this\",\"line\",\"will\",\"be\",\"skipped\"";
    ImportTests::set_parser_contents(parser, raw_line, sizeof(raw_line));

    while(parser.on_idle_parse())
    {}

    bool passed = (get_tokens_instance().size() == 0);
    if(!ImportTests::check("test_skip_on_no_ending_newline", passed, report))
      result = false;

    get_tokens_instance().clear();
    parser.clear();
  }

  // test_skip_on_no_quotes_around_token
  {
    const char raw_line[] = "this,line,contains,only,empty,tokens\n";
    ImportTests::set_parser_contents(parser, raw_line, sizeof(raw_line));

    while(parser.on_idle_parse())
    {}

    bool passed = check_tokens("^(|,)$");
    if(!ImportTests::check("test_skip_on_no_quotes_around_token", passed, report))
      result = false;

    get_tokens_instance().clear();
    parser.clear();
  }

  // test_skip_spaces_around_separators
  {
    const char raw_line[] = "\"spaces\" , \"around\", \"separators\"\n";
    ImportTests::set_parser_contents(parser, raw_line, sizeof(raw_line));

    while(parser.on_idle_parse())
    {}

    bool passed = (get_tokens_instance().size() == 5);
    if(!ImportTests::check("test_skip_spaces_around_separators", passed, report))
      result = false;

    get_tokens_instance().clear();
    parser.clear();
  }

  // test_fail_on_non_comma_separators
  {
    const char raw_line[] = "\"cannot\"\t\"tokenize\"\t\"this\"\n";
    ImportTests::set_parser_contents(parser, raw_line, sizeof(raw_line));

    while(parser.on_idle_parse())
    {}

    bool passed = check_tokens("^cannottokenizethis$");
    if(!ImportTests::check("test_fail_on_non_comma_separators", passed, report))
      result = false;

    get_tokens_instance().clear();
    parser.clear();
  }

  // test_parse_newline_inside_quotes
  {
    const char raw_line[] = "\"cell with\nnewline\"\n\"token on next line\"";
    ImportTests::set_parser_contents(parser, raw_line, sizeof(raw_line));

    while(parser.on_idle_parse())
    {}

    bool passed = check_tokens("^(cell with\nnewline|token on next line)$");
    if(!ImportTests::check("test_parse_newline_inside_quotes", passed, report))
      result = false;

    get_tokens_instance().clear();
    parser.clear();
  }

  if(!result)
    std::cout << report.rdbuf() << std::endl;

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
