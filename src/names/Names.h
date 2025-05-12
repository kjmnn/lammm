#ifndef NAMES_H
#define NAMES_H

#include <string_view>

namespace lammm::names {

/// @brief Symbols
namespace symbol {
constexpr char open_paren = '(';
constexpr char close_paren = ')';
constexpr char open_square = '[';
constexpr char close_square = ']';
constexpr char space = ' ';
constexpr char plus = '+';
constexpr char minus = '-';
constexpr char star = '*';
constexpr char slash = '/';
constexpr char modulo = '%';
constexpr char question = '?';
constexpr char colon = ':';
} // namespace symbol

/// @brief Syntax keywords
namespace keyword {
constexpr std::string_view def = "def";
constexpr std::string_view case_ = "case";
constexpr std::string_view cocase = "cocase";
constexpr std::string_view ifz = "ifz";
constexpr std::string_view mu_c_ascii = "mu'";
constexpr std::string_view mu_c_unicode = "μ'";
constexpr std::string_view mu_p_ascii = "mu";
constexpr std::string_view mu_p_unicode = "μ";
constexpr std::string_view end = "<END>";
} // namespace keyword

/// @brief Names of syntax element types
namespace AST {
constexpr std::string_view variable = "variable";
constexpr std::string_view value = "value";
constexpr std::string_view mu_c = "mu abstraction";
constexpr std::string_view constructor = "constructor";
constexpr std::string_view constructor_expr = "constructor expression";
constexpr std::string_view cocase = "cocase expression";
constexpr std::string_view covariable = "covariable";
constexpr std::string_view mu_p = "mu' abstraction";
constexpr std::string_view destructor = "destructor";
constexpr std::string_view destructor_expr = "destructor expression";
constexpr std::string_view case_ = "case expression";
constexpr std::string_view end = "end of computation";
constexpr std::string_view arithmetic = "arithmetic statement";
constexpr std::string_view ifz = "if-zero statement";
constexpr std::string_view cut = "cut statement";
constexpr std::string_view call = "call statement";
constexpr std::string_view producer = "producer";
constexpr std::string_view consumer = "consumer";
constexpr std::string_view statement = "statement";
constexpr std::string_view definition = "definition";
constexpr std::string_view def_or_stmt = "definition or statement";
constexpr std::string_view clause = "clause";
constexpr std::string_view case_clause = "case clause";
constexpr std::string_view cocase_clause = "cocase clause";
constexpr std::string_view parameter = "parameter";
constexpr std::string_view coparameter = "coparameter";
constexpr std::string_view argument = "argument";
constexpr std::string_view coargument = "coargument";
} // namespace AST

namespace constant {
    constexpr std::string_view integer_ABC = "abc";
}

/// @brief Miscellaneous fragments
namespace misc {
constexpr std::string_view arity = "arity";
constexpr std::string_view coarity = "coarity";
}

/// @brief Names of builtin types
namespace type {
constexpr std::string_view integer = "Integer";
constexpr std::string_view list = "List";
constexpr std::string_view pair = "Pair";
constexpr std::string_view stream = "Stream";
constexpr std::string_view lazy_pair = "LazyPair";
constexpr std::string_view lambda = "Lambda";
} // namespace type

/// @brief Names of builtin (con/de)structors
namespace structor {
constexpr std::string_view nil = "Nil";
constexpr std::string_view cons = "Cons";
constexpr std::string_view pair = "Pair";
constexpr std::string_view head = "Head";
constexpr std::string_view tail = "Tail";
constexpr std::string_view fst = "Fst";
constexpr std::string_view snd = "Snd";
constexpr std::string_view ap = "Ap";
} // namespace structor

/// @brief Names of builtin definition(s)
namespace definition {
constexpr std::string_view print = "Print";
}

} // namespace lammm::names

#endif