#ifndef TESTS_COMMON_H
#define TESTS_COMMON_H
#include "../AST.h"
#include <string_view>

namespace lammm::tests {

/// @brief Simple statement to test arithmetic, ifz and mu' abstractions
constexpr std::string_view stmt_ifz_simple =
    "(- 2 2 (mu' x (ifz x [123 <END>] [x <END>])))\n";

/// @brief A silly definition to showcase namespace separation
constexpr std::string_view def_silly = "(def foo (foo) (foo) [foo foo])";

/// @brief Map @p f over @p xs and feed result into @p then
constexpr std::string_view def_list_map =
    "(def ListMap (f xs) (then)\n"
    "  [xs\n"
    "   (case ((Nil         [(Nil) then])\n"
    "          (Cons (x xs) [(Cons ((mu xThen [f (Ap (x) (xThen))])\n"
    "                               (mu xsThen (ListMap (f xs) (xsThen)))))\n"
    "                        then])))])\n";

/// @brief Sum elements of @p p and feed result into @p then
constexpr std::string_view def_pair_sum =
    "(def PairSum (p) (then)\n"
    "  [p (case ((Pair (a b) (+ a b then))))])\n";

/// @brief Combine @p ListMap and @p PairSum to sum elements of a list of pairs
constexpr std::string_view stmt_map_sum_pair =
    "(ListMap ((cocase ((Ap (p) (then) (PairSum (p) (then)))))\n"    // f
    "          (Cons ((Pair (1 2)) (Cons ((Pair (3 4)) (Nil))))))\n" // xs
    "         (<END>))\n";                                           // then

/// @brief Ill-typed statement - polymorphic list
constexpr std::string_view stmt_poly_list_bad =
    "[(Cons (1 (Cons ((Nil) (Nil))))) <END>]";

/// @brief Ill-typed definition - polymorphic recursion
constexpr std::string_view def_poly_recursion_bad = //
    "(def PolyRec (x) () \n"
    "   (PolyRec ((Pair (x x))) ()))";

} // namespace lammm::tests

#endif