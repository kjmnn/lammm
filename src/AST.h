#ifndef AST_H
#define AST_H

#include "Types.h"
#include "Util.h"
#include <optional>
#include <variant>
#include <vector>

namespace lammm {

struct Variable_prod;
struct Value_prod;
struct Mu_prod;
struct Constructor_prod;
struct Cocase_prod;
/// @brief One of the syntax categories; corresponds to things that evaluate to values
using Producer = std::variant<Box<Variable_prod>, Box<Value_prod>, Box<Mu_prod>,
                              Box<Constructor_prod>, Box<Cocase_prod>>;

struct Covariable_cons;
struct Mu_cons;
struct Destructor_cons;
struct Case_cons;
struct End_cons;
/// @brief One of the syntax categories; corresponds to continuations
using Consumer =
    std::variant<Box<Covariable_cons>, Box<Mu_cons>, Box<Destructor_cons>,
                 Box<Case_cons>, Box<End_cons>>;

struct Arithmetic_stmt;
struct Ifz_stmt;
struct Cut_stmt;
struct Call_stmt;
/// @brief One of the syntax categories; links producers and consumers with potential extra effects
using Statement = std::variant<Box<Arithmetic_stmt>, Box<Ifz_stmt>,
                               Box<Cut_stmt>, Box<Call_stmt>>;

/// @brief Variable ID, for typing & runtime replacement
struct Var_id
{
    std::size_t id;
};

inline bool operator<(const Var_id& a, const Var_id& b)
{
    return a.id < b.id;
}

/// @brief Covariable ID, for typing & runtime replacement
struct Covar_id
{
    std::size_t id;
};

inline bool operator<(const Covar_id& a, const Covar_id& b)
{
    return a.id < b.id;
}

/// @brief Definition ID, indexes @p Program::definitions
struct Definition_id
{
    std::size_t id;
};

inline bool operator<(const Definition_id& a, const Definition_id& b)
{
    return a.id < b.id;
}

/// @brief Represents a variable
struct Variable_prod
{
    Var_id var_id;
    std::string var_name;
    std::optional<Type_handle> type;
};

/// @brief Represents an integer literal
struct Value_prod
{
    std::int64_t value;
    std::optional<Type_handle> type;
};

/// @brief Represents a mu abstraction (a general value-producing expression)
struct Mu_prod
{
    Covar_id coarg_id;
    std::string coarg_name;
    Statement body;
    std::optional<Type_handle> type;
};

/// @brief Represents a constructor (produces data)
struct Constructor_prod
{
    Abstraction_id abstraction_id;
    std::string constructor_name;
    std::vector<Producer> args;
    std::vector<Consumer> coargs;
    std::optional<bool> is_value;
    std::optional<Type_handle> type;
};

/// @brief Represents a clause of a case consumer or cocase producer
struct Clause
{
    Abstraction_id abstraction_id;
    std::string structor_name;
    std::vector<std::string> arg_names;
    std::vector<std::string> coarg_names;
    std::vector<Var_id> arg_ids;
    std::vector<Covar_id> coarg_ids;
    Statement body;
};

/// @brief Represents a cocase expression (produces codata)
struct Cocase_prod
{
    std::vector<Clause> clauses;
    std::optional<Type_handle> type;
};

/// @brief Represents a covariable
struct Covariable_cons
{
    Covar_id covar_id;
    std::string covar_name;
    std::optional<Type_handle> type;
};

/// @brief Represents a mu' abstraction (essentially a general continuation)
struct Mu_cons
{
    Var_id arg_id;
    std::string arg_name;
    Statement body;
    std::optional<Type_handle> type;
};

/// @brief Represents a destructor (consumes codata)
struct Destructor_cons
{
    Abstraction_id abstraction_id;
    std::string destructor_name;
    std::vector<Producer> args;
    std::vector<Consumer> coargs;
    std::optional<Type_handle> type;
};

/// @brief Represents a case expression (consumes data)
struct Case_cons
{
    std::vector<Clause> clauses;
    std::optional<Type_handle> type;
};

/// @brief Represents the end of computation
struct End_cons
{
    std::optional<Type_handle> type;
};

/// @brief Arithmetic operators
enum class Arithmetic_op
{
    add,
    sub,
    mul,
    div,
    mod
};

/// @brief Represents an arithmetic statement
struct Arithmetic_stmt
{
    Arithmetic_op op;
    Producer left;
    Producer right;
    Consumer after;
};

/// @brief Represents an if-zero statement
struct Ifz_stmt
{
    Producer condition;
    Statement if_zero;
    Statement if_other;
};

/// @brief Represents a cut statement (simplest way to link producers and consumers)
struct Cut_stmt
{
    Producer producer;
    Consumer consumer;
};

/// @brief Represents a call statement (to a Definition)
struct Call_stmt
{
    /// @brief Definition ID for code lookup
    Definition_id definition_id;
    /// @brief Name of the definition
    std::string definition_name;
    std::vector<Producer> args;
    std::vector<Consumer> coargs;
};

/// @brief Represents a top-level definition
struct Definition
{
    /// @brief Abstraction ID for typing
    Abstraction_id abstraction_id;
    /// @brief Name of the definition
    std::string definition_name;
    std::vector<std::string> arg_names;
    std::vector<std::string> coarg_names;
    std::vector<Var_id> arg_ids;
    std::vector<Covar_id> coarg_ids;
    Statement body;
};

/// @brief Represents a whole program
struct Program
{
    std::vector<Definition> definitions;
    std::vector<Statement> statements;
};

/// @brief Pointer to a syntax element (used for error reporting)
using Syntax_ptr = std::variant<Producer*, Consumer*, Statement*, Clause*,
                                Definition*, Program*>;

} // namespace lammm

#endif