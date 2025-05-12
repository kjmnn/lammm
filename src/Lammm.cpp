#include "Interpreter.h"
#include "Parser.h"
#include "Printer.h"
#include "Typer.h"
#include "Types.h"

using namespace lammm;

int main(int argc, char** argv)
{
    auto ctx = default_typing_context();
    auto parser = Parser{ctx};
    Program program;
    try {
        program = parser.parse_program(std::cin);
    } catch (const Exception& e) {
        std::cerr << "Error parsing program:" << std::endl;
        std::cerr << e.name() << ": " << e.message() << std::endl;
        return 1;
    }
    try {
        type_program(program, ctx);
    } catch (const Exception& e) {
        std::cerr << "Error typing program:" << std::endl;
        std::cerr << e.name() << ": " << e.message() << std::endl;
        return 2;
    }
    try {
        Interpreter interpreter{parser.n_vars(),
                                parser.n_covars(),
                                std::move(program),
                                {.print_definitions = true,
                                 .print_start = true,
                                 .print_intermediate = true,
                                 .print_results = true,
                                 .print_info = true,
                                 .print_types = false},
                                std::cout,
                                &ctx};
        interpreter.run();
    } catch (const Exception& e) {
        std::cerr << "Error running program:" << std::endl;
        std::cerr << e.name() << ": " << e.message() << std::endl;
        return 3;
    }
    return 0;
}