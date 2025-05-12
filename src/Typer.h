#ifndef TYPER_H
#define TYPER_H
#include "AST.h"
#include "Types.h"
#include <set>
#include <vector>

namespace lammm {

class Typer
{
  public:
    /// @brief Typer constructor
    /// @param ctx Typing context (should be the same as the one used for parsing)
    /// @param definitions Definitions for typing call statements
    Typer(Typing_context& ctx, const std::vector<Definition>& definitions);

    /// @brief Check the type of a producer
    /// @param prod Producer to typecheck
    /// @param type Expected type 
    /// @throw @p Single_typing_exception if unification fails
    void check(Producer& prod, Strong_type_handle type);
    /// @brief Check the type of a consumer
    /// @param prod Consumer to typecheck
    /// @param type Expected type 
    /// @throw @p Single_typing_exception if unification fails
    void check(Consumer& cons, Strong_type_handle type);
    /// @brief Typecheck a statement
    /// @param stmt Statement to typecheck
    /// @throw @p Single_typing_exception if unification fails
    void check(Statement& stmt);
    /// @brief Check the type of a clause
    /// @param clause Clause to typecheck
    /// @param type Expected type
    /// @throw @p Single_typing_exception if unification fails
    void check(Clause& clause, Strong_type_handle type);
    /// @brief Typecheck a definition
    /// @param definition Definition to typecheck
    /// @throw @p Single_typing_exception if unification fails
    void check(Definition& definition);

  private:
    Typing_context& ctx;
    const std::vector<Definition>& definitions;
    Strong_type_handle int_type;
    std::map<Var_id, Strong_type_handle> var_types;
    std::map<Covar_id, Strong_type_handle> covar_types;
    std::optional<Abstraction_id> current_definition;

    /// @brief Check the type of an abstraction instance (structor application or definition call)
    /// @param id Abstraction ID
    /// @param args Arguments
    /// @param coargs Coarguments
    /// @param context Enclosing syntax element (for error reporting)
    /// @param type Expected type (none for definitions)
    /// @return result type (if not none)
    /// @throw @p Single_typing_exception if unification fails
    std::optional<Type_handle> check_abstraction(
        Abstraction_id id, std::vector<Producer>& args,
        std::vector<Consumer>& coargs, Syntax_ptr context,
        std::optional<Strong_type_handle> type = std::nullopt);
    /// @brief Generate a fresh type variable for a variable;
    ///        Checks that the variable is not already bound
    /// @param id Variable ID
    void fresh_var(Var_id id);
    /// @brief Generate a fresh type variable for a covariable;
    ///        Checks that the covariable is not already bound
    /// @param id Covariable ID
    void fresh_covar(Covar_id id);
    /// @brief Try to unify two types, rethrow with more information on failure
    /// @param a First type to unify
    /// @param b Second type to unify
    /// @param context Enclosing syntax element (for error reporting)
    /// @throw @p Single_typing_exception if unification fails
    void try_unify(Strong_type_handle a, Strong_type_handle b,
                   Syntax_ptr context);
};

/// @brief Create a fresh typer and typecheck a program
/// @param program Program to typecheck
/// @param ctx Typing context
/// @throw @p Multiple_typing_exception if errors occur
void type_program(Program& program, Typing_context& ctx);

/// @brief Type error base class
class Typing_exception : public Exception
{
    std::string name() const override;
};

/// @brief Single type error
class Single_typing_exception : public Typing_exception
{
  public:
    Single_typing_exception(Unification_exception cause, Syntax_ptr context)
        : cause{cause}, context{context}
    {
    }

    std::string message() const override;

  private:
    /// @brief Unification error thrown by @p ctx.unify (cheap to copy)
    Unification_exception cause;
    /// @brief Syntax element where the error occurred
    Syntax_ptr context;
};

/// @brief Aggregate type error thrown by @p type_program
class Multiple_typing_exception : public Typing_exception
{
  public:
    Multiple_typing_exception(std::vector<Single_typing_exception>&& errors)
        : errors{std::move(errors)}
    {
    }

    std::string message() const override;

  private:
    /// @brief The constituent errors
    std::vector<Single_typing_exception> errors;
};

} // namespace lammm

#endif