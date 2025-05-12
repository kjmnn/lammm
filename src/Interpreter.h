#ifndef INTERPRETER_H
#define INTERPRETER_H
#include "AST.h"
#include "Util.h"
#include <iostream>

namespace lammm {

/// @brief Options struct for @p Interpreter
struct Interpreter_options
{
    /// @brief Print definitions befor running
    bool print_definitions = false;
    /// @brief Print each statement before executing it
    bool print_start = false;
    /// @brief Print intermediate results
    bool print_intermediate = false;
    /// @brief Print final results
    bool print_results = false;
    /// @brief Print extra information (e.g. reduction rules used)
    bool print_info = false;
    /// @brief Print types when printing definitions and initial statements
    bool print_types = false;
};

/// @brief An interpreter for Lammm programs.
class Interpreter
{
  public:
    /// @brief Interpreter constructor
    /// @param n_vars (Initial) no. of variables
    /// @param n_covars (Initial) no. of covariables
    /// @param program The program to interpret! Assumed to be syntactically correct
    /// @param options Interpreter options
    Interpreter(std::size_t n_vars, std::size_t n_covars, Program&& program,
                Interpreter_options options, std::ostream& stream = std::cout, 
                Typing_context* ctx = nullptr)
        : n_vars{n_vars},
          n_covars{n_covars},
          definitions{std::move(program.definitions)},
          statements{std::move(program.statements)},
          options{options},
          stream{stream},
          ctx{ctx}
    {
    }

    /// @brief Run the program! (non-repeatable, mutates internal state)
    /// @return Results of program statements - what got passed into @p <END>
    /// @throw @p Stuck_computation_exception if computation gets stuck;
    ///           Note that this should never occur with a correctly typed program.
    /// @throw @p Already_run_exception if the interpreter has already been run
    std::vector<Producer> run();

  private:
    using Step_result = std::variant<Producer, Statement>;

    /// @brief Interpreter options
    Interpreter_options options;
    /// @brief Typing context (for display purposes)
    Typing_context* ctx;
    /// @brief Whether the interpreter has been run and finished
    bool finished = false;
    /// @brief Output stream
    std::ostream& stream;
    /// @brief Current number of variables (used to generate IDs when focusing)
    std::size_t n_vars;
    /// @brief Current number of covariables (used to generate IDs when focusing)
    std::size_t n_covars;
    /// @brief Program definitions
    std::vector<Definition> definitions;
    /// @brief Program statements
    std::vector<Statement> statements;

    /// @brief Perform a reduction step on a statement
    /// @param stmt Statement to reduce
    /// @return Reduction result (producer in the [_ <END>] case, statement otherwise)
    /// @throw @p Stuck_computation_exception if there is no possible reduction
    Step_result step(Statement&& stmt);
    Step_result step(Box<Arithmetic_stmt>&& stmt);
    Step_result step(Box<Ifz_stmt>&& stmt);
    Step_result step(Box<Cut_stmt>&& stmt);
    Step_result step(Box<Call_stmt>&& stmt);

    /// @brief Perform an arithmetic operation
    /// @param op Arithmetic operator
    /// @param left Left operand
    /// @param right Right operand
    /// @return Operation result 
    static std::int64_t do_arithmetic(Arithmetic_op op, std::int64_t left,
                                      std::int64_t right);

    /// @brief Common implementation for clause matching;
    /// @param abstraction_id Matched structor ID
    /// @param args Matched structor arguments
    /// @param coargs Matched structor coarguments
    /// @param clauses List of clauses to match against
    /// @param context Enclosing statement (for error reporting)
    /// @return Body of matching clause with the proper substitutions
    Statement eval_clauses(Abstraction_id abstraction_id,
                                  std::vector<Producer>&& args,
                                  std::vector<Consumer>&& coargs,
                                  std::vector<Clause>&& clauses,
                                  Box<Cut_stmt>& context);

    /// @brief Transform a constructor with a non-value argument into a
    ///        mu abstraction to allow further evaluation
    /// @param prod Constructor to focus
    /// @param arg_index Index of first non-value argument
    /// @return Focusing result
    Box<Mu_prod> focus_constructor(Box<Constructor_prod>&& prod,
                                   std::size_t arg_index);
    /// @brief Transform a destructor with a non-value argument into a
    ///        mu' abstraction to allow further evaluation
    /// @param cons Destructor to focus
    /// @param arg_index Index of first non-value argument
    /// @return Focusing result
    Box<Mu_cons> focus_destructor(Box<Destructor_cons>&& cons,
                                  std::size_t arg_index);
    /// @brief Transform an arithmetic statement with a non-value operand
    ///        into a different statement that can be evaluated
    /// @param stmt Cut statement to focus
    /// @param focus_left Whether to focus on the left or right operand
    /// @return Focusing result
    Box<Cut_stmt> focus_arithmetic(Box<Arithmetic_stmt>&& stmt,
                                   bool focus_left);
    /// @brief Transform an ifz statement with a non-value condition
    ///        into a different statement that can be evaluated
    /// @param stmt Ifz statement to focus
    /// @return Focusing result
    Box<Cut_stmt> focus_ifz(Box<Ifz_stmt>&& stmt);
    /// @brief Transform a call statement with a non-value argument
    ///        into a different statement that can be evaluated
    /// @param stmt Call statement to focus
    /// @param arg_index Index of first non-value argument
    /// @return Focusing result
    Box<Cut_stmt> focus_call(Box<Call_stmt>&& stmt, std::size_t arg_index);
    /// @brief Common focusing sub-operation
    /// @param stmt Statement to focus
    /// @param prod Non-value occuring somewhere in @p stmt
    /// @param new_var_name Name of the variable that will replace @p prod
    /// @return Focusing result
    Box<Cut_stmt> focus_statement(Statement&& stmt, Producer&& prod,
                                  std::string&& new_var_name);

    /// @brief Find the first non-value argument in a list of producers
    /// @param args List of arguments (producers) to check
    /// @return The non-value's index, if found
    static std::optional<std::size_t> find_non_value(
        std::vector<Producer>& args);

    /// @brief Bind variables and covariables by replacement
    /// @param prod Producer to bind in
    /// @param var_map Map of variable IDs to their new values
    /// @param covar_map Map of covariable IDs to their new values
    static void replace(Producer& prod,
                        const std::map<Var_id, Producer*>& var_map,
                        const std::map<Covar_id, Consumer*>& covar_map);
    /// @brief Bind variables and covariables by replacement
    /// @param cons Consumer to bind in
    /// @param var_map Map of variable IDs to their new values
    /// @param covar_map Map of covariable IDs to their new values
    static void replace(Consumer& cons,
                        const std::map<Var_id, Producer*>& var_map,
                        const std::map<Covar_id, Consumer*>& covar_map);
    /// @brief Bind variables and covariables by replacement
    /// @param stmt Statement to bind in
    /// @param var_map Map of variable IDs to their new values
    /// @param covar_map Map of covariable IDs to their new values
    static void replace(Statement& stmt,
                        const std::map<Var_id, Producer*>& var_map,
                        const std::map<Covar_id, Consumer*>& covar_map);
    /// @brief Bind variables and covariables by replacement
    /// @param clause Clause to bind in
    /// @param var_map Map of variable IDs to their new values
    /// @param covar_map Map of covariable IDs to their new values
    static void replace(Clause& clause,
                        const std::map<Var_id, Producer*>& var_map,
                        const std::map<Covar_id, Consumer*>& covar_map);

    /// @brief Generate a new variable ID
    /// @return The new variable ID
    Var_id fresh_var_id();
    /// @brief Generate a new covariable ID
    /// @return The new covariable ID
    Covar_id fresh_covar_id();

    /// @brief Print information if @p options.print_info is set
    /// @param info Information to print
    void print_info(std::string&& info);
};

/// @brief Base class for interpreter exceptions
class Interpreter_exception : public Exception
{
  public:
    std::string name() const override;
};

/// @brief Thrown when an interpreter is run for the second time
class Already_run_exception : public Interpreter_exception
{
  public:
    std::string message() const override;
};

/// @brief Thrown when computation gets stuck;
///        Should never be thrown when interpreting a typechecked program
class Stuck_computation_exception : public Interpreter_exception
{
  public:
    Stuck_computation_exception(Statement&& context)
        : context{context}
    {
    }

    std::string message() const override;

  private:
    /// @brief The stuck statement
    Statement context;
};

/// @brief Check that a producer is a value
///        (i.e. a @p Value_prod or a @p Cocase_prod or a constructor with only value arguments);
///        Results are cached for constructors (the only producer whose value status can change)
/// @param prod Producer to check
/// @return Whether the producer is a value (in the above sense)
bool is_value(Producer& prod);

} // namespace lammm

#endif