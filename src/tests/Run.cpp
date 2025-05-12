#include "Programs.h"
#include "Tests.h"
#include <iostream>
#include <sstream>

using namespace lammm;

int main(int, char**)
{
    std::ostringstream test_oss;
    test_oss << tests::def_silly << tests::def_list_map << tests::def_pair_sum
             << tests::stmt_map_sum_pair << tests::stmt_ifz_simple;
    std::string test_program = test_oss.str();
    bool all_ok = true;
    all_ok &= tests::parses_ok(test_program, "test_program_parses_ok");
    all_ok &= tests::pppp(test_program, "test_program_pppp");
    all_ok &= tests::typechecks(test_program, "test_program_typechecks");
    all_ok &= tests::ill_typed_stmt(std::string(tests::stmt_poly_list_bad),
                                    "poly_list_type_bad");
    all_ok &= tests::ill_typed_def(std::string(tests::def_poly_recursion_bad),
                                   "poly_recursion_type_bad");
    all_ok &= tests::runs_ok(test_program, "test_program_runs_ok");
    if (all_ok) {
        std::cout << "ALL TESTS PASSED" << std::endl;
        return 0;
    } else {
        std::cerr << "SOME TESTS FAILED" << std::endl;
        return 1;
    }
}
