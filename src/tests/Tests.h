#ifndef TESTS_H
#define TESTS_H
#include "Programs.h"
#include <string>
#include <string_view>

namespace lammm::tests {

/// @brief Test that a program parses without errors
/// @param program Test program
/// @param test_name Name of the test for result reporting
/// @return whether the test passed
bool parses_ok(const std::string& program, std::string_view test_name);

/// @brief The parse-print-parse-print test - runs two passes of parsing and printing
///        and checks that the printed programs are the same
/// @param program Test program
/// @param test_name Name of the test for result reporting
/// @return whether the test passed
bool pppp(const std::string& program, std::string_view test_name);

/// @brief Test that a program parses without errors and typechecks
/// @param program Test program
/// @param test_name Name of the test for result reporting
/// @return whether the test passed
bool typechecks(const std::string& program, std::string_view test_name);

/// @brief Test that a definition is ill-typed
/// @param definition Test definition
/// @param test_name Name of the test for result reporting
/// @return whether the test passed
bool ill_typed_def(const std::string& definition, std::string_view test_name);

/// @brief Test that a statement is ill-typed
/// @param definition Test statement
/// @param test_name Name of the test for result reporting
/// @return whether the test passed
bool ill_typed_stmt(const std::string& statement, std::string_view test_name);

/// @brief Test that a program runs without errors
/// @param program Test program
/// @param test_name Name of the test for result reporting
/// @return whether the test passed
bool runs_ok(const std::string& program, std::string_view test_name);

} // namespace lammm::tests

#endif