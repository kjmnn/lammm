#include "Interpreter.h"
#include "Printer.h"
#include "names/Names.h"
#include <algorithm>
#include <cassert>
#include <format>
#include <sstream>

using namespace lammm::names;

namespace lammm {

// Reductions, focusing & other info

constexpr std::string_view info_definitions = "-- Definitions --";
constexpr std::string_view info_start = "-- Evaluating next statement --";
constexpr std::string_view info_arithmetic = "-- Reduce: Arithmetic --";
constexpr std::string_view info_arithmetic_focus_l =
    "-- Focus: Arithmetic (left) --";
constexpr std::string_view info_arithmetic_focus_r =
    "-- Focus: Arithmetic (right) --";
constexpr std::string_view info_ifz_focus = "-- Focus: If-zero --";
constexpr std::string_view info_ifz_zero = "-- Reduce: If-zero (zero) --";
constexpr std::string_view info_ifz_other = "-- Reduce: If-zero (other) --";
constexpr std::string_view info_mu_p = "-- Reduce: Mu --";
constexpr std::string_view info_constructor_focus =
    "-- Focus: Constructor {} ({}) --";
constexpr std::string_view info_mu_c = "-- Reduce: Mu' --";
constexpr std::string_view info_case = "-- Reduce: Case {} --";
constexpr std::string_view info_destructor_focus =
    "-- Focus: Destructor {} ({}) --";
constexpr std::string_view info_cocase = "-- Reduce: Cocase {} --";
constexpr std::string_view info_finished = "-- Finished! --";
constexpr std::string_view info_call_focus = "-- Focus: Call {} ({}) --";
constexpr std::string_view info_call = "-- Reduce: Call {} --";

// Focusing variable names

constexpr std::string_view focus_var_template = "_{}_{}";
constexpr std::string_view focus_var_arith_l = "_ar_l";
constexpr std::string_view focus_var_arith_r = "_ar_r";
constexpr std::string_view focus_var_ifz = "_ifz";

std::vector<Producer> Interpreter::run()
{
    if (finished) {
        throw Already_run_exception{};
    }
    std::vector<Producer> results;
    if (options.print_definitions) {
        print_info(std::string(info_definitions));
        for (auto& definition : definitions) {
            print(definition, {.print_types = options.print_types}, stream, ctx);
            stream << std::endl;
        }
    }
    // Run all statements in sequence
    for (auto& stmt : statements) {
        std::size_t steps = 0;
        Step_result current = std::move(stmt);
        if (options.print_start) {
            print_info(std::string(info_start));
            print(std::get<Statement>(current),
                  {.print_types = options.print_types}, stream, ctx);
            stream << std::endl;
        }
        // Run computation steps until we reach a result (producer)
        while (std::holds_alternative<Statement>(current)) {
            if (options.print_intermediate && steps > 0) {
                print(std::get<Statement>(current), {}, stream);
                stream << std::endl;
            }
            ++steps;
            current = step(std::move(std::get<Statement>(current)));
        }
        auto result = std::get<Producer>(current);
        if (options.print_results) {
            print(result, {}, stream);
            stream << std::endl;
        }
        results.emplace_back(std::move(result));
    }
    finished = true;
    return results;
}

Interpreter::Step_result Interpreter::step(Statement&& stmt)
{
    return std::visit( //
        overloaded{
            [this](Box<Arithmetic_stmt>&& stmt) {
                return step(std::move(stmt)); //
            },
            [this](Box<Ifz_stmt>&& stmt) {
                return step(std::move(stmt)); //
            },
            [this](Box<Cut_stmt>&& stmt) {
                return step(std::move(stmt)); //
            },
            [this](Box<Call_stmt>&& stmt) {
                return step(std::move(stmt)); //
            },
        },
        std::move(stmt));
}

Interpreter::Step_result Interpreter::step(Box<Arithmetic_stmt>&& stmt)
{
    if (!is_value(stmt->left)) {
        print_info(std::string(info_arithmetic_focus_l));
        return focus_arithmetic(std::move(stmt), true);
    } else if (!is_value(stmt->right)) {
        print_info(std::string(info_arithmetic_focus_r));
        return focus_arithmetic(std::move(stmt), false);
    }
    auto op = stmt->op;
    return std::visit(
        overloaded{
            [this, &op](Box<Value_prod>&& left, Box<Value_prod>&& right,
                        auto&& cons) {
                // Compute & cut
                print_info(std::string(info_arithmetic));
                return Statement{box(Cut_stmt{
                    .producer = box(Value_prod{
                        .value = do_arithmetic(op, left->value, right->value),
                    }),
                    .consumer = std::move(cons),
                })};
            },
            [&stmt](auto&&, auto&&, auto&&) {
                // Ill-typed args, we are stuck
                throw Stuck_computation_exception{std::move(stmt)};
                // Unreachable, but needed to typecheck
                return Statement{stmt};
            },
        },
        std::move(stmt->left), std::move(stmt->right), std::move(stmt->after));
}

Interpreter::Step_result Interpreter::step(Box<Ifz_stmt>&& stmt)
{
    if (!is_value(stmt->condition)) {
        print_info(std::string(info_ifz_focus));
        return focus_ifz(std::move(stmt));
    }
    if (std::holds_alternative<Box<Value_prod>>(stmt->condition)) {
        // Return selected branch
        if (std::get<Box<Value_prod>>(stmt->condition)->value == 0) {
            print_info(std::string(info_ifz_zero));
            return std::move(stmt->if_zero);
        } else {
            print_info(std::string(info_ifz_other));
            return std::move(stmt->if_other);
        }
    } else {
        // Ill-typed condition, we are stuck
        throw Stuck_computation_exception{std::move(stmt)};
    }
}

Interpreter::Step_result Interpreter::step(Box<Cut_stmt>&& stmt)
{
    if (std::holds_alternative<Box<Mu_prod>>(stmt->producer)) {
        // mu abstraction has the highest priority
        auto& mu = std::get<Box<Mu_prod>>(stmt->producer);
        // Replace occurences of the bound covariable with the consumer
        std::map<Covar_id, Consumer*> covar_map{
            {mu->coarg_id, &stmt->consumer}};
        auto body = std::move(mu->body);
        replace(body, {}, covar_map);
        print_info(std::string(info_mu_p));
        return body;
    } else if (!is_value(stmt->producer)) {
        // The producer can't be a mu abstraction,
        // so the only option left should be a non-value constructor
        if (!std::holds_alternative<Box<Constructor_prod>>(stmt->producer)) {
            // [variable _] case, we are stuck as a variable is not a value
            throw Stuck_computation_exception{std::move(stmt)};
        }
        auto& constructor = std::get<Box<Constructor_prod>>(stmt->producer);
        auto non_value_arg = find_non_value(constructor->args);
        // Sanity check
        assert(non_value_arg.has_value());
        stmt->producer =
            focus_constructor(std::move(constructor), non_value_arg.value());
        print_info(std::format(info_constructor_focus,
                               constructor->constructor_name,
                               non_value_arg.value()));
        return std::move(stmt);
    } else if (std::holds_alternative<Box<Mu_cons>>(stmt->consumer)) {
        // The producer is a value, so we can proceed
        // (I wanted to put this into the std::visit below, but it wouldn't typecheck)
        auto& mu = std::get<Box<Mu_cons>>(stmt->consumer);
        // Replace occurences of the bound variable with the producer
        std::map<Var_id, Producer*> var_map{{mu->arg_id, &stmt->producer}};
        auto body = std::move(mu->body);
        replace(body, var_map, {});
        print_info(std::string(info_mu_c));
        return body;
    }
    return std::visit(
        overloaded{
            [this, &stmt](Box<Constructor_prod>&& constructor,
                          Box<Case_cons>&& case_cons) {
                auto res = eval_clauses(constructor->abstraction_id,
                                        std::move(constructor->args),
                                        std::move(constructor->coargs),
                                        std::move(case_cons->clauses), stmt);
                print_info(
                    std::format(info_case, constructor->constructor_name));
                return Step_result{std::move(res)};
            },
            [this, &stmt](Box<Cocase_prod>&& cocase_prod,
                          Box<Destructor_cons>&& destructor) {
                auto non_value_arg =
                    find_non_value(destructor->args);
                if (non_value_arg.has_value()) {
                    // Focus on the first non-value argument
                    print_info(std::format(info_destructor_focus,
                                           destructor->destructor_name,
                                           non_value_arg.value()));
                    stmt->consumer = focus_destructor(std::move(destructor),
                                            non_value_arg.value());
                    return Step_result{std::move(stmt)};
                }
                auto res = eval_clauses(destructor->abstraction_id,
                                        std::move(destructor->args),
                                        std::move(destructor->coargs),
                                        std::move(cocase_prod->clauses), stmt);
                print_info(
                    std::format(info_cocase, destructor->destructor_name));
                return Step_result{std::move(res)};
            },
            [this](auto&& value, Box<End_cons>&&) {
                // Return the result
                print_info(std::string(info_finished));
                return Step_result{std::move(value)};
            },
            [&stmt](auto&&, auto&&) {
                // Mismatched cut, we are stuck
                throw Stuck_computation_exception{std::move(stmt)};
                // Unreachable, but needed to typecheck
                return Step_result{stmt};
            },
        },
        std::move(stmt->producer), std::move(stmt->consumer));
}

Interpreter::Step_result Interpreter::step(Box<Call_stmt>&& stmt)
{
    // Check if all arguments are values
    auto non_value = find_non_value(stmt->args);
    if (non_value.has_value()) {
        // Focus on the first non-value argument
        print_info(std::format(info_call_focus, stmt->definition_name,
                               non_value.value()));
        return focus_call(std::move(stmt), non_value.value());
    }
    // All arguments are values, we can proceed
    auto& definition = definitions.at(stmt->definition_id.id);
    // Sanity check (we assume the program is syntactically correct)
    assert(definition.arg_ids.size() == stmt->args.size());
    assert(definition.coarg_ids.size() == stmt->coargs.size());
    // Create (co)arg maps
    std::map<Var_id, Producer*> var_map;
    for (std::size_t i = 0; i < stmt->args.size(); ++i) {
        var_map.emplace(definition.arg_ids[i], &stmt->args[i]);
    }
    std::map<Covar_id, Consumer*> covar_map;
    for (std::size_t i = 0; i < stmt->coargs.size(); ++i) {
        covar_map.emplace(definition.coarg_ids[i], &stmt->coargs[i]);
    }
    // Copy definition body
    auto body = definition.body;
    // Replace (co)parameters with (co)arguments
    replace(body, var_map, covar_map);
    print_info(std::format(info_call, definition.definition_name));
    return body;
}

std::int64_t Interpreter::do_arithmetic(Arithmetic_op op, std::int64_t left,
                                        std::int64_t right)
{
    switch (op) {
        case Arithmetic_op::add:
            return left + right;
        case Arithmetic_op::sub:
            return left - right;
        case Arithmetic_op::mul:
            return left * right;
        case Arithmetic_op::div:
            if (right != 0) {
                return left / right;
            } else {
                // A bit odd, but makes the semantics nicer
                return 1;
            }
        case Arithmetic_op::mod:
            if (right != 0) {
                return left % right;
            } else {
                // A bit odd, but makes the semantics nicer
                return left;
            }
        default:
            // Unreachable
            assert(false);
    }
}

Statement Interpreter::eval_clauses(Abstraction_id abstraction_id,
                                    std::vector<Producer>&& args,
                                    std::vector<Consumer>&& coargs,
                                    std::vector<Clause>&& clauses,
                                    Box<Cut_stmt>& context)
{
    auto matching =
        std::find_if(clauses.begin(), clauses.end(),
                     [&abstraction_id](const Clause& clause) {
                         return clause.abstraction_id == abstraction_id;
                     });
    if (matching == clauses.end()) {
        // No matching clause, we are stuck
        throw Stuck_computation_exception{std::move(context)};
    }
    // Sanity checks (we assume the program is syntactically correct)
    assert(matching->arg_ids.size() == args.size());
    assert(matching->coarg_ids.size() == coargs.size());
    // Create (co)arg maps
    std::map<Var_id, Producer*> var_map;
    for (std::size_t i = 0; i < args.size(); ++i) {
        var_map.emplace(matching->arg_ids[i], &args[i]);
    }
    std::map<Covar_id, Consumer*> covar_map;
    for (std::size_t i = 0; i < coargs.size(); ++i) {
        covar_map.emplace(matching->coarg_ids[i], &coargs[i]);
    }
    // Copy clause body
    auto body = matching->body;
    // Replace (co)parameters with (co)arguments
    replace(body, var_map, covar_map);
    return body;
}

Box<Mu_prod> Interpreter::focus_constructor(Box<Constructor_prod>&& prod,
                                            std::size_t arg_index)
{
    // Save reference to the non-value argument for later
    // (should survive all the vector moves just fine)
    auto& non_value = prod->args[arg_index];
    // Wrap the constructor in a cut statement
    auto new_covar_id = fresh_covar_id();
    auto new_covar_name =
        std::format(focus_var_template, prod->constructor_name, arg_index);
    // Save a copy of the constructor name for later
    std::string constructor_name = prod->constructor_name;
    auto inner_cut = box(Cut_stmt{
        .producer = std::move(prod),
        .consumer = box(Covariable_cons{
            .covar_id = new_covar_id,
            .covar_name = new_covar_name,
        }),
    });
    // Wrap the cut in a mu' abstraction and another cut
    auto outer_cut = focus_statement(
        std::move(inner_cut), std::move(non_value),
        std::format(focus_var_template, constructor_name, arg_index));
    // And wrap that in a mu abstraction
    return box(Mu_prod{
        .coarg_id = new_covar_id,
        .coarg_name = std::move(new_covar_name),
        .body = std::move(outer_cut),
    });
}

Box<Mu_cons> Interpreter::focus_destructor(Box<Destructor_cons>&& cons,
                                           std::size_t arg_index)
{
    // Save reference to the non-value argument for later
    // (should survive all the vector moves just fine)
    auto& non_value = cons->args[arg_index];
    // Wrap the destructor in a cut statement
    auto new_var_id = fresh_var_id();
    auto new_var_name =
        std::format(focus_var_template, cons->destructor_name, arg_index);
    auto inner_cut = box(Cut_stmt{
        .producer = box(Variable_prod{
            .var_id = new_var_id,
            .var_name = new_var_name,
        }),
        .consumer = std::move(cons),
    });
    // Wrap the cut in a mu' abstraction and another cut
    auto outer_cut = focus_statement(
        std::move(inner_cut), std::move(non_value),
        std::format(focus_var_template, cons->destructor_name, arg_index));
    // And wrap that in another mu' abstraction
    return box(Mu_cons{
        .arg_id = new_var_id,
        .arg_name = std::move(new_var_name),
        .body = std::move(outer_cut),
    });
}

Box<Cut_stmt> Interpreter::focus_arithmetic(Box<Arithmetic_stmt>&& stmt,
                                            bool focus_left)
{
    return focus_statement(
        std::move(stmt),
        focus_left ? std::move(stmt->left) : std::move(stmt->right),
        std::string(focus_left ? focus_var_arith_l : focus_var_arith_r));
}

Box<Cut_stmt> Interpreter::focus_ifz(Box<Ifz_stmt>&& stmt)
{
    return focus_statement(std::move(stmt), std::move(stmt->condition),
                           std::string(focus_var_ifz));
}

Box<Cut_stmt> Interpreter::focus_call(Box<Call_stmt>&& stmt,
                                      std::size_t arg_index)
{
    return focus_statement(
        std::move(stmt), std::move(stmt->args[arg_index]),
        std::format(focus_var_template, stmt->definition_name, arg_index));
}

Box<Cut_stmt> Interpreter::focus_statement(Statement&& stmt, Producer&& prod,
                                           std::string&& new_var_name)
{
    // Replace producer with a new variable
    auto cut_prod = std::move(prod);
    auto new_var_id = fresh_var_id();
    prod = box(Variable_prod{
        .var_id = new_var_id,
        .var_name = new_var_name,
    });
    // Wrap the statement in a mu' abstraction
    auto cut_cons = box(Mu_cons{
        .arg_id = new_var_id,
        .arg_name = std::move(new_var_name),
        .body = std::move(stmt),
    });
    // Return the cut statement (might lead to more focusing right after)
    return box(Cut_stmt{
        .producer = std::move(cut_prod),
        .consumer = std::move(cut_cons),
    });
}

void Interpreter::replace(Producer& prod,
                          const std::map<Var_id, Producer*>& var_map,
                          const std::map<Covar_id, Consumer*>& covar_map)
{
    std::visit( //
        overloaded{
            [&var_map, &prod](Box<Variable_prod>& var_prod) {
                if (var_map.contains(var_prod->var_id)) {
                    // Replace the variable with a copy of the corresponding value
                    prod = *var_map.at(var_prod->var_id);
                }
            },
            [](Box<Value_prod>& prod) {
                // Nothing to replace
            },
            [&var_map, &covar_map](Box<Mu_prod>& prod) {
                // Remove the no longer free covariable
                auto new_covar_map = covar_map;
                new_covar_map.erase(prod->coarg_id);
                replace(prod->body, var_map, new_covar_map);
            },
            [&var_map, &covar_map](Box<Constructor_prod>& prod) {
                if (prod->is_value == false) {
                    // Variables might get replaced by values,
                    // turning the constructor expression into a value
                    prod->is_value.reset();
                }
                for (auto& arg : prod->args) {
                    replace(arg, var_map, covar_map);
                }
                for (auto& coarg : prod->coargs) {
                    replace(coarg, var_map, covar_map);
                }
            },
            [&var_map, &covar_map](Box<Cocase_prod>& prod) {
                for (auto& clause : prod->clauses) {
                    replace(clause, var_map, covar_map);
                }
            },
        },
        prod);
}

void Interpreter::replace(Consumer& cons,
                          const std::map<Var_id, Producer*>& var_map,
                          const std::map<Covar_id, Consumer*>& covar_map)
{
    std::visit( //
        overloaded{
            [&covar_map, &cons](Box<Covariable_cons>& covar_cons) {
                if (covar_map.contains(covar_cons->covar_id)) {
                    // Replace the covariable with a copy of the corresponding covalue
                    cons = *covar_map.at(covar_cons->covar_id);
                }
            },
            [&var_map, &covar_map](Box<Mu_cons>& cons) {
                // Remove the no longer free variable
                auto new_var_map = var_map;
                new_var_map.erase(cons->arg_id);
                replace(cons->body, new_var_map, covar_map);
            },
            [&var_map, &covar_map](Box<Destructor_cons>& cons) {
                for (auto& arg : cons->args) {
                    replace(arg, var_map, covar_map);
                }
                for (auto& coarg : cons->coargs) {
                    replace(coarg, var_map, covar_map);
                }
            },
            [&var_map, &covar_map](Box<Case_cons>& cons) {
                for (auto& clause : cons->clauses) {
                    replace(clause, var_map, covar_map);
                }
            },
            [](Box<End_cons>&) {
                // Nothing to replace
            },
        },
        cons);
}

void Interpreter::replace(Statement& stmt,
                          const std::map<Var_id, Producer*>& var_map,
                          const std::map<Covar_id, Consumer*>& covar_map)
{
    std::visit( //
        overloaded{
            [&var_map, &covar_map](Box<Arithmetic_stmt>& stmt) {
                replace(stmt->left, var_map, covar_map);
                replace(stmt->right, var_map, covar_map);
                replace(stmt->after, var_map, covar_map);
            },
            [&var_map, &covar_map](Box<Ifz_stmt>& stmt) {
                replace(stmt->condition, var_map, covar_map);
                replace(stmt->if_zero, var_map, covar_map);
                replace(stmt->if_other, var_map, covar_map);
            },
            [&var_map, &covar_map](Box<Cut_stmt>& stmt) {
                replace(stmt->producer, var_map, covar_map);
                replace(stmt->consumer, var_map, covar_map);
            },
            [&var_map, &covar_map](Box<Call_stmt>& stmt) {
                for (auto& arg : stmt->args) {
                    replace(arg, var_map, covar_map);
                }
                for (auto& coarg : stmt->coargs) {
                    replace(coarg, var_map, covar_map);
                }
            },
        },
        stmt);
}

void Interpreter::replace(Clause& clause,
                          const std::map<Var_id, Producer*>& var_map,
                          const std::map<Covar_id, Consumer*>& covar_map)
{
    // Remove no longer free (co)variables
    auto new_var_map = var_map;
    auto new_covar_map = covar_map;
    for (auto arg : clause.arg_ids) {
        new_var_map.erase(arg);
    }
    for (auto coarg : clause.coarg_ids) {
        new_covar_map.erase(coarg);
    }
    // Replace (co)variables in the body
    replace(clause.body, new_var_map, new_covar_map);
}

Var_id Interpreter::fresh_var_id()
{
    return Var_id{n_vars++};
}

Covar_id Interpreter::fresh_covar_id()
{
    return Covar_id{n_covars++};
}

void Interpreter::print_info(std::string&& info)
{
    if (options.print_info) {
        stream << info << std::endl;
    }
}

std::optional<std::size_t> Interpreter::find_non_value(
    std::vector<Producer>& args)
{
    auto non_value = std::find_if_not(args.begin(), args.end(), is_value);
    if (non_value != args.end()) {
        // Return the index of the first non-value argument
        return non_value - args.begin();
    } else {
        // All arguments are values
        return std::nullopt;
    }
}

bool is_value(Producer& prod)
{
    return std::visit( //
        overloaded{
            [](Box<Value_prod>&) { return true; },
            [](Box<Cocase_prod>&) { return true; },
            [](Box<Constructor_prod>& prod) {
                if (!prod->is_value.has_value()) {
                    // Check if all arguments are values and cache the result
                    prod->is_value = std::all_of(prod->args.begin(),
                                                 prod->args.end(), is_value);
                }
                return prod->is_value.value();
            },
            [](auto) { return false; },
        },
        prod);
}

std::string Interpreter_exception::name() const
{
    return "Interpreter error";
}

std::string Already_run_exception::message() const
{
    return "Interpreter has already run";
}

std::string Stuck_computation_exception::message() const
{
    std::ostringstream msgs;
    msgs << "No reduction or focusing rule found for statement:\n";
    print(context, {}, msgs);
    return msgs.str();
}

} // namespace lammm