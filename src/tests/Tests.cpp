#include "Tests.h"
#include "../Interpreter.h"
#include "../Parser.h"
#include "../Printer.h"
#include "../Typer.h"
#include <sstream>

namespace lammm::tests {

bool parses_ok(const std::string& program, std::string_view test_name)
{
    std::istringstream input{program};
    auto ctx = default_typing_context();
    Parser parser{ctx};
    try {
        parser.parse_program(input);
        std::cout << "Test passed: " << test_name << std::endl;
        return true;
    } catch (Parse_exception& e) {
        std::cerr << "Parse error during test: " << test_name << std::endl;
        std::cerr << e.name() << ": " << e.message() << std::endl;
        return false;
    }
}

bool pppp(const std::string& program, std::string_view test_name)
{
    std::string first;
    std::string second;
    try {
        // First pass
        std::istringstream input_1{program};
        auto ctx_1 = default_typing_context();
        Parser parser_1{ctx_1};
        auto parsed_1 = parser_1.parse_program(input_1);
        std::ostringstream output_1{first};
        print(parsed_1, {}, output_1, &ctx_1);
        // Second pass
        std::istringstream input_2{program};
        auto ctx_2 = default_typing_context();
        Parser parser_2{ctx_2};
        auto parsed_2 = parser_2.parse_program(input_2);
        std::ostringstream output_second{second};
        print(parsed_2, {}, output_second, &ctx_2);
    } catch (Parse_exception& e) {
        std::cerr << "Parse error during test: " << test_name << std::endl;
        std::cerr << e.name() << ": " << e.message() << std::endl;
        return false;
    }
    if (first == second) {
        std::cout << "Test passed: " << test_name << std::endl;
        return true;
    } else {
        std::cerr << "Test failed: " << test_name << std::endl;
        std::cerr << "After first pass: \n" << first << std::endl;
        std::cerr << "After second pass: \n" << second << std::endl;
        return false;
    }
}

bool typechecks(const std::string& program, std::string_view test_name)
{
    std::istringstream input{program};
    auto ctx = default_typing_context();
    Parser parser{ctx};
    try {
        auto program = parser.parse_program(input);
        type_program(program, ctx);
        std::cout << "Test passed: " << test_name << std::endl;
        return true;
    } catch (Exception& e) {
        std::cerr << "Error during test: " << test_name << std::endl;
        std::cerr << e.name() << ": " << e.message() << std::endl;
        return false;
    }
}

bool ill_typed_def(const std::string& definition, std::string_view test_name)
{
    std::istringstream input{definition};
    auto ctx = default_typing_context();
    Parser parser{ctx};
    try {
        auto definition = parser.parse_definition(input);
        std::vector<Definition> definitions;
        definitions.emplace_back(std::move(definition));
        Typer typer{ctx, definitions};
        typer.check(definitions[0]);
        std::cerr << "Test failed (expected an exception): " << test_name
                  << std::endl;
        return false;
    } catch (Typing_exception& e) {
        std::cout << "Test passed: " << test_name << std::endl;
        return true;
    } catch (Exception& e) {
        std::cerr << "Unexpected error during test: " << test_name << std::endl;
        std::cerr << e.name() << ": " << e.message() << std::endl;
        return false;
    }
}

bool ill_typed_stmt(const std::string& statement, std::string_view test_name)
{
    std::istringstream input{statement};
    auto ctx = default_typing_context();
    Parser parser{ctx};
    try {
        auto statement = parser.parse_statement(input);
        Typer typer{ctx, {}};
        typer.check(statement);
        std::cerr << "Test failed (expected an exception): " << test_name
                  << std::endl;
        return false;
    } catch (Typing_exception& e) {
        std::cout << "Test passed: " << test_name << std::endl;
        return true;
    } catch (Exception& e) {
        std::cerr << "Unexpected error during test: " << test_name << std::endl;
        std::cerr << e.name() << ": " << e.message() << std::endl;
        return false;
    }
}

bool runs_ok(const std::string& program, std::string_view test_name)
{
    std::istringstream input{program};
    auto ctx = default_typing_context();
    Parser parser{ctx};
    try {
        auto program = parser.parse_program(input);
        type_program(program, ctx);
        Interpreter interpreter{
            parser.n_vars(), parser.n_covars(), std::move(program), {}};
        interpreter.run();
        std::cout << "Test passed: " << test_name << std::endl;
        return true;
    } catch (Exception& e) {
        std::cerr << "Error during test: " << test_name << std::endl;
        std::cerr << e.name() << ": " << e.message() << std::endl;
        return false;
    }
}

} // namespace lammm::tests