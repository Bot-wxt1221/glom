#ifndef TEST_IMPORT_UTILS_H
#define TEST_IMPORT_UTILS_H

#include <glom/import_csv/csv_parser.h>
//#include <sigc++/sigc++.h>
#include <iostream>
#include <unistd.h>
#include <errno.h>

namespace ImportTests
{

bool check(const std::string& name, bool test, std::stringstream& report);

typedef sigc::slot<void, Glom::CsvParser&> FuncConnectParserSignals;

/**
 * @result Whether the parser finished without being killed by a timeout.
 */
bool run_parser_from_buffer(const FuncConnectParserSignals& connect_parser_signals, const char* input, guint input_size);

/**
 * @result Whether the parser finished without being killed by a timeout.
 */
bool run_parser_from_buffer(const FuncConnectParserSignals& connect_parser_signals, const std::string& input);

} //namespace ImportTests
#endif
