#include "Typer.h"
#include "Printer.h"
#include "Util.h"
#include <cassert>
#include <map>
#include <sstream>

namespace lammm {

Typer::Typer(Typing_context& ctx, const std::vector<Definition>& definitions)
    : ctx{ctx}, definitions{definitions}
{
    int_type = ctx.get_primitive_prototype(Builtin_type_id::integer);
}

void Typer::check(Producer& prod, Strong_type_handle type)
{
    std::visit( //
        overloaded{
            [this, &prod, type](Box<Variable_prod>& var_prod) {
                // All variable occurences must be of the same type
                try_unify(type, var_types[var_prod->var_id], &prod);
                var_prod->type = var_types[var_prod->var_id];
            },
            [this, &prod, type](Box<Value_prod>& value_prod) {
                // Values must be integers
                try_unify(type, int_type, &prod);
                value_prod->type = int_type;
            },
            [this, &prod, type](Box<Mu_prod>& mu_prod) {
                // Mu abstractions must be of the same type
                fresh_covar(mu_prod->coarg_id);
                // This unification should never fail, the covar type should be fresh
                try_unify(type, covar_types[mu_prod->coarg_id], &prod);
                check(mu_prod->body);
                mu_prod->type = covar_types[mu_prod->coarg_id];
            },
            [this, &prod, type](Box<Constructor_prod>& constructor_prod) {
                // Delegate to abstraction check
                constructor_prod->type =
                    check_abstraction(constructor_prod->abstraction_id,
                                      constructor_prod->args,
                                      constructor_prod->coargs, &prod, type)
                        .value();
            },
            [this, type](Box<Cocase_prod>& cocase_prod) {
                // Check all clauses
                for (auto& clause : cocase_prod->clauses) {
                    check(clause, type);
                }
            },
        },
        prod);
}

void Typer::check(Consumer& cons, Strong_type_handle type)
{
    std::visit( //
        overloaded{
            [this, &cons, type](Box<Covariable_cons>& covar_cons) {
                // Covariables must be of the same type
                try_unify(type, covar_types[covar_cons->covar_id], &cons);
                covar_cons->type = covar_types[covar_cons->covar_id];
            },
            [this, &cons, type](Box<Mu_cons>& mu_cons) {
                // Mu abstractions must be of the same type
                fresh_var(mu_cons->arg_id);
                // This unification should never fail, the var type should be fresh
                try_unify(type, var_types[mu_cons->arg_id], &cons);
                check(mu_cons->body);
                mu_cons->type = var_types[mu_cons->arg_id];
            },
            [this, &cons, type](Box<Destructor_cons>& destructor_cons) {
                // Delegate to abstraction check
                destructor_cons->type =
                    check_abstraction(destructor_cons->abstraction_id,
                                      destructor_cons->args,
                                      destructor_cons->coargs, &cons, type)
                        .value();
            },
            [this, type](Box<Case_cons>& cons) {
                // Check all clauses
                for (auto& clause : cons->clauses) {
                    check(clause, type);
                }
            },
            [this, type](Box<End_cons>& cons) {
                // End can be of any type
                cons->type = type;
            },
        },
        cons);
}

void Typer::check(Statement& stmt)
{
    std::visit( //
        overloaded{
            [this, &stmt](Box<Arithmetic_stmt>& box_stmt) {
                // Both the operands and the consumer must be integers
                check(box_stmt->left, int_type);
                check(box_stmt->right, int_type);
                check(box_stmt->after, int_type);
            },
            [this, &stmt](Box<Ifz_stmt>& ifz_stmt) {
                // Condition must be an integer
                check(ifz_stmt->condition, int_type);
                check(ifz_stmt->if_zero);
                check(ifz_stmt->if_other);
            },
            [this, &stmt](Box<Cut_stmt>& cut_stmt) {
                // The producer and consumer types must match
                auto cut_type = ctx.fresh_type_variable();
                check(cut_stmt->producer, cut_type);
                check(cut_stmt->consumer, cut_type);
            },
            [this, &stmt](Box<Call_stmt>& call_stmt) {
                // Delegate to abstraction check
                check_abstraction(
                    definitions.at(call_stmt->definition_id.id).abstraction_id,
                    call_stmt->args, call_stmt->coargs, &stmt);
            },
        },
        stmt);
}

void Typer::check(Clause& clause, Strong_type_handle type)
{
    for (auto& arg : clause.arg_ids) {
        fresh_var(arg);
    }
    for (auto& coarg : clause.coarg_ids) {
        fresh_covar(coarg);
    }
    auto instance = ctx.instantiate(clause.abstraction_id);
    // Sanity checks
    assert(instance.type.has_value());
    assert(instance.args.size() == clause.arg_ids.size());
    assert(instance.coargs.size() == clause.coarg_ids.size());
    // Check the type of the clause
    // Even though we check totality & matching in the parser,
    // the complete types of the clauses can still differ
    // due to differing type parameters
    try_unify(type, instance.type.value(), &clause);
    // Typecheck the arguments and coarguments
    for (std::size_t i = 0; i < instance.args.size(); ++i) {
        // Can't fail, the arg vars are fresh
        try_unify(var_types[clause.arg_ids[i]], instance.args[i], &clause);
    }
    for (std::size_t i = 0; i < instance.coargs.size(); ++i) {
        // Ditto for coargs
        try_unify(covar_types[clause.coarg_ids[i]], instance.coargs[i],
                  &clause);
    }
    check(clause.body);
}

void Typer::check(Definition& definition)
{
    // Type the definition (co)params
    for (auto& arg : definition.arg_ids) {
        fresh_var(arg);
    }
    for (auto& coarg : definition.coarg_ids) {
        fresh_covar(coarg);
    }
    auto& abstraction = ctx.get_abstraction(definition.abstraction_id);
    // Make note of the current definition to type recursive calls correctly
    current_definition = definition.abstraction_id;
    // Sanity checks
    assert(!abstraction.type.has_value());
    assert(abstraction.args.size() == definition.arg_ids.size());
    assert(abstraction.coargs.size() == definition.coarg_ids.size());
    // Unify abstraction args and coargs with the definition args and coargs
    for (std::size_t i = 0; i < abstraction.args.size(); ++i) {
        // Can't fail, the arg vars are fresh
        try_unify(var_types[definition.arg_ids[i]],
                  // We know what we're doing here
                  Strong_type_handle{abstraction.args[i].id}, &definition);
    }
    for (std::size_t i = 0; i < abstraction.coargs.size(); ++i) {
        // Ditto for coargs
        try_unify(covar_types[definition.coarg_ids[i]],
                  // Here too
                  Strong_type_handle{abstraction.coargs[i].id}, &definition);
    }
    // Check the body (needs to be done last, because the definition might be recursive)
    check(definition.body);
    // Unset current definition
    current_definition.reset();
}

std::optional<Type_handle> Typer::check_abstraction(
    Abstraction_id id, std::vector<Producer>& args,
    std::vector<Consumer>& coargs, Syntax_ptr context,
    std::optional<Strong_type_handle> type)
{
    Abstraction_instance instance;
    if (current_definition == id) {
        // Recursive call, which means we can't generalize the defintion's type
        instance = ctx.get_abstraction_prototype(id);
    } else {
        // Get a fresh instance of the definition
        instance = ctx.instantiate(id);
    }
    // Sanity checks
    assert(instance.type.has_value() == type.has_value());
    // (any potential arity mismatch should be caught in the parser)
    assert(instance.args.size() == args.size());
    assert(instance.coargs.size() == coargs.size());
    // Typecheck the arguments and coarguments
    for (std::size_t i = 0; i < instance.args.size(); ++i) {
        check(args[i], instance.args[i]);
    }
    for (std::size_t i = 0; i < instance.coargs.size(); ++i) {
        check(coargs[i], instance.coargs[i]);
    }
    // Check type if present
    if (type.has_value()) {
        try_unify(type.value(), instance.type.value(), context);
    }
    return instance.type;
}

void Typer::fresh_var(Var_id id)
{
    // Correctly constructed programs should only contain
    // a single binder for each variable
    assert(!var_types.contains(id));
    var_types.emplace(id, ctx.fresh_type_variable());
}

void Typer::fresh_covar(Covar_id id)
{
    // Ditto for covariables
    assert(!covar_types.contains(id));
    covar_types.emplace(id, ctx.fresh_type_variable());
}

void Typer::try_unify(Strong_type_handle a, Strong_type_handle b,
                      Syntax_ptr context)
{
    try {
        ctx.unify(a, b);
    } catch (const Unification_exception& e) {
        throw Single_typing_exception{e, context};
    }
}

void type_program(Program& program, Typing_context& ctx)
{
    Typer typer{ctx, program.definitions};
    std::vector<Single_typing_exception> errors;
    for (auto& definition : program.definitions) {
        try {
            typer.check(definition);
        } catch (const Single_typing_exception& e) {
            errors.push_back(e);
        }
    }
    for (auto& statement : program.statements) {
        try {
            typer.check(statement);
        } catch (const Single_typing_exception& e) {
            errors.push_back(e);
        }
    }
    if (!errors.empty()) {
        throw Multiple_typing_exception{std::move(errors)};
    }
}

std::string Typing_exception::name() const
{
    return "Type error";
}

std::string Single_typing_exception::message() const
{
    std::ostringstream msgs;
    msgs << "While typing ";
    Printer printer{{}, msgs, nullptr};
    std::visit(printer, context);
    msgs << ": " << cause.message();
    return msgs.str();
}

std::string Multiple_typing_exception::message() const
{
    if (errors.size() == 1) {
        return errors[0].message();
    }
    // If there are multiple errors, give each one its own line
    std::ostringstream msgs;
    for (auto& err : errors) {
        msgs << '\n' << err.message();
    }
    return msgs.str();
}

} // namespace lammm