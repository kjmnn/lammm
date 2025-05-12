#include "Types.h"
#include "Printer.h"
#include "names/Names.h"
#include <cassert>
#include <format>
#include <sstream>
#include <string_view>

namespace lammm {

Type_id Typing_context::add_type_prototype(std::string_view name,
                                           std::size_t n_params,
                                           std::optional<Type_id> id)
{
    if (id.has_value()) {
        assert(id.value() == type_prototypes.size());
    }
    auto prototype_handle = types.size();
    // Create type & parameters
    types.reserve(types.size() + n_params + 1);
    types.emplace_back(Concrete_type{.type_id = type_prototypes.size()});
    auto& new_type = std::get<Concrete_type>(types.back());
    for (std::size_t i = 0; i < n_params; ++i) {
        new_type.params.push_back(fresh_type_variable());
    }
    // Register type
    type_prototypes.push_back(Type_handle{prototype_handle});
    type_names.emplace_back(name);
    return type_prototypes.size() - 1;
}

Abstraction_id Typing_context::add_structor(std::string_view name,
                                            Type_id type_id,
                                            std::vector<Type_template>&& args,
                                            std::vector<Type_template>&& coargs,
                                            std::optional<Abstraction_id> id)
{
    if (id.has_value()) {
        assert(id.value() == abstractions.size());
    }
    assert(type_id.id < type_prototypes.size());
    assert(std::holds_alternative<Concrete_type>(
        types[type_prototypes[type_id.id].id]));
    // Get handles to parameters of constructed type (copy intentional)
    auto params =
        std::get<Concrete_type>(types[type_prototypes[type_id.id].id]).params;
    // Instantiate arg & coarg type templates
    std::vector<Type_handle> arg_handles;
    arg_handles.reserve(args.size());
    for (auto& arg : args) {
        arg_handles.push_back(instantiate_type_template(arg, params));
    }
    std::vector<Type_handle> coarg_handles;
    coarg_handles.reserve(coargs.size());
    for (auto& coarg : coargs) {
        coarg_handles.push_back(instantiate_type_template(coarg, params));
    }
    // Create & register structor
    abstractions.emplace_back(Abstraction{.type = type_prototypes[type_id.id],
                                          .abstraction_name{name},
                                          .args = std::move(arg_handles),
                                          .coargs = std::move(coarg_handles)});
    // Register structor in the type structors map
    type_structors[type_prototypes[type_id.id]].insert(
        Abstraction_id{abstractions.size() - 1});
    return abstractions.size() - 1;
}

Abstraction_id Typing_context::add_definition(std::string_view name,
                                              std::size_t arity,
                                              std::size_t coarity,
                                              std::optional<Abstraction_id> id)
{
    if (id.has_value()) {
        assert(id.value() == abstractions.size());
    }
    // Create parameter types
    std::vector<Type_handle> args;
    args.reserve(arity);
    for (std::size_t i = 0; i < arity; ++i) {
        args.push_back(fresh_type_variable());
    }
    std::vector<Type_handle> coargs;
    coargs.reserve(coarity);
    for (std::size_t i = 0; i < coarity; ++i) {
        coargs.push_back(fresh_type_variable());
    }
    // Create & register definition
    abstractions.emplace_back(Abstraction{.abstraction_name{name},
                                          .args = std::move(args),
                                          .coargs = std::move(coargs)});
    return abstractions.size() - 1;
}

const Type_instance& Typing_context::get_type_instance(Type_handle ref) const
{
    return types[try_dereference(ref).id];
}

Type_handle Typing_context::get_type_prototype(Type_id id) const
{
    return type_prototypes[id.id];
}

Strong_type_handle Typing_context::get_primitive_prototype(Type_id id) const
{
    // A type is primitive iff it has no parameters
    assert(std::get<Concrete_type>(types[type_prototypes[id.id].id])
               .params.empty());
    return Strong_type_handle{type_prototypes[id.id].id};
}

const Abstraction& Typing_context::get_abstraction(Abstraction_id id) const
{
    return abstractions[id.id];
}

const Abstraction_instance Typing_context::get_abstraction_prototype(
    Abstraction_id id) const
{
    auto& abstraction = abstractions[id.id];
    std::vector<Strong_type_handle> args;
    args.reserve(abstraction.arity());
    for (auto& arg : abstraction.args) {
        args.push_back(Strong_type_handle{arg.id});
    }
    std::vector<Strong_type_handle> coargs;
    coargs.reserve(abstraction.coarity());
    for (auto& coarg : abstraction.coargs) {
        coargs.push_back(Strong_type_handle{coarg.id});
    }
    return Abstraction_instance{.abstraction_name =
                                    abstraction.abstraction_name,
                                .args = std::move(args),
                                .coargs = std::move(coargs)};
}

const std::string& Typing_context::get_type_name(Type_id id) const
{
    return type_names[id.id];
}

const std::set<Abstraction_id>& Typing_context::structors_like(
    Abstraction_id id) const
{
    return type_structors.at(abstractions[id.id].type.value());
}

Abstraction_instance Typing_context::instantiate(Abstraction_id abstraction_id)
{
    auto& abstraction = abstractions[abstraction_id.id];
    // Collect relevant types
    std::vector<Type_handle> to_clone;
    to_clone.reserve(abstraction.arity() + abstraction.coarity() + 1);
    for (auto& arg : abstraction.args) {
        to_clone.push_back(arg);
    }
    for (auto& coarg : abstraction.coargs) {
        to_clone.push_back(coarg);
    }
    if (abstraction.type.has_value()) {
        to_clone.push_back(abstraction.type.value());
    }
    // Clone them
    auto fresh_types = clone_types(std::move(to_clone));
    // And repack into an Abstraction_instance
    Abstraction_instance instance;
    if (abstraction.type.has_value()) {
        instance.type = fresh_types.back();
    }
    instance.args.reserve(abstraction.arity());
    for (std::size_t i = 0; i < abstraction.arity(); ++i) {
        instance.args.push_back(fresh_types[i]);
    }
    instance.coargs.reserve(abstraction.coarity());
    for (std::size_t i = 0; i < abstraction.coarity(); ++i) {
        instance.coargs.push_back(fresh_types[abstraction.arity() + i]);
    }
    return instance;
}

Strong_type_handle Typing_context::fresh_type_variable()
{
    types.emplace_back(Type_var{types.size() - 1});
    return Strong_type_handle{types.size() - 1};
}

void Typing_context::unify(Strong_type_handle a, Strong_type_handle b)
{
    unify_rec(a, b);
}

void Typing_context::unify_rec(Type_handle a, Type_handle b)
{
    // Follow forwards
    a = try_dereference(a);
    b = try_dereference(b);
    // Trivial end
    if (a == b) {
        return;
    }
    // eliminate the (concrete, var) case
    if (std::holds_alternative<Type_var>(types[b.id])) {
        std::swap(a, b);
    }
    if (std::holds_alternative<Type_var>(types[a.id])) {
        // (var, _) case
        if (occurs(a, b)) {
            throw Unification_exception{Unification_exception::Kind::occurs,
                                        *this, a, b};
        } else {
            types[a.id] = b;
        }
    } else {
        // (concrete, concrete) case is the only one left
        assert(std::holds_alternative<Concrete_type>(types[a.id]));
        assert(std::holds_alternative<Concrete_type>(types[b.id]));
        auto& a_concrete = std::get<Concrete_type>(types[a.id]);
        auto& b_concrete = std::get<Concrete_type>(types[b.id]);
        if (a_concrete.type_id != b_concrete.type_id) {
            throw Unification_exception{Unification_exception::Kind::mismatch,
                                        *this, a, b};
        } else {
            assert(a_concrete.params.size() == b_concrete.params.size());
            for (std::size_t i = 0; i < a_concrete.params.size(); ++i) {
                // We know what we're doing
                unify_rec(a_concrete.params[i], b_concrete.params[i]);
            }
        }
    }
}

bool Typing_context::occurs(Type_handle a, Type_handle b)
{
    assert(std::holds_alternative<Type_var>(types[a.id]));
    b = try_dereference(b);
    auto& b_type = types[b.id];
    if (std::holds_alternative<Type_var>(b_type)) {
        return a == b;
    } else {
        assert(std::holds_alternative<Concrete_type>(b_type));
        auto& b_concrete = std::get<Concrete_type>(b_type);
        for (auto& param : b_concrete.params) {
            if (param == a) {
                return true;
            } else if (occurs(a, param)) {
                return true;
            }
        }
        return false;
    }
}

std::vector<Strong_type_handle> Typing_context::clone_types(
    std::vector<Type_handle>&& types)
{
    std::map<std::size_t, Strong_type_handle> map;
    std::vector<Strong_type_handle> result;
    result.reserve(types.size());
    for (auto& type : types) {
        result.push_back(clone_type_rec(type, map));
    }
    return result;
}

Strong_type_handle Typing_context::clone_type_rec(
    Type_handle type, std::map<std::size_t, Strong_type_handle>& map)
{
    type = try_dereference(type);
    if (map.contains(type.id)) {
        return map[type.id];
    }
    return std::visit( //
        overloaded{
            [this, &map, &type](const Type_var&) {
                auto new_handle = fresh_type_variable();
                map.emplace(type.id, new_handle);
                return new_handle;
            },
            [this, &map, &type](Concrete_type concrete) {
                // Create the new (cloned) type & register the mapping
                Strong_type_handle new_handle{types.size()};
                types.emplace_back(Concrete_type{.type_id = concrete.type_id});
                map.emplace(type.id, new_handle);
                // Clone parameters (into a new vector, as the type vector might grow, invalidating references)
                std::vector<Type_handle> new_params;
                // new_params.reserve(concrete.params.size());
                for (auto& param : concrete.params) {
                    new_params.push_back(clone_type_rec(param, map));
                }
                // Set the new type's parameters
                std::get<Concrete_type>(types[new_handle.id]).params =
                    std::move(new_params);
                return new_handle;
            },
            [](const Type_handle& handle) {
                // Unreachable
                assert(false);
                return Strong_type_handle{0};
            },
        },
        types[type.id]);
}

Type_handle Typing_context::instantiate_type_template(
    const Type_template& template_, const std::vector<Type_handle>& params)
{
    return std::visit( //
        overloaded{
            [&params](const Type_template_var& var) {
                assert(var.id < params.size());
                return params[var.id];
            },
            [this, &params](const Box<Concrete_type_template>& concrete) {
                std::vector<Type_handle> new_params;
                new_params.reserve(concrete->params.size());
                for (auto& param : concrete->params) {
                    new_params.push_back(
                        instantiate_type_template(param, params));
                }
                types.emplace_back(
                    Concrete_type{.type_id = concrete->type_id,
                                  .params = std::move(new_params)});
                return Type_handle{types.size() - 1};
            },
        },
        template_);
}

Type_handle Typing_context::try_dereference(Type_handle type)
{
    if (!std::holds_alternative<Type_handle>(types[type.id])) {
        return type;
    }
    // Follow the reference chain
    std::vector<Type_handle> reference_chain;
    while (std::holds_alternative<Type_handle>(types[type.id])) {
        reference_chain.push_back(type);
        type = std::get<Type_handle>(types[type.id]);
    }
    // Compress the reference chain (last one is already direct)
    reference_chain.pop_back();
    for (auto& reference : reference_chain) {
        types[reference.id] = type;
    }
    assert(!std::holds_alternative<Type_handle>(types[type.id]));
    return type;
}

Type_handle Typing_context::try_dereference(Type_handle type) const
{
    if (!std::holds_alternative<Type_handle>(types[type.id])) {
        return type;
    }
    // Follow the reference chain
    while (std::holds_alternative<Type_handle>(types[type.id])) {
        type = std::get<Type_handle>(types[type.id]);
    }
    assert(!std::holds_alternative<Type_handle>(types[type.id]));
    return type;
}

std::string Unification_exception::name() const
{
    return "Unification error";
}

std::string Unification_exception::message() const
{
    std::ostringstream as;
    print(a, {}, as, &ctx);
    std::ostringstream bs;
    print(b, {}, bs, &ctx);
    if (kind == Kind::occurs) {
        return std::format("type {} occurs in {}", as.str(), bs.str());
    } else {
        return std::format("{} and {} have different type constructors",
                           as.str(), bs.str());
    }
}

// -BUILTIN TYPES-

using Builtin_spec = std::tuple<std::string_view, std::size_t, Builtin_type_id>;
constexpr Builtin_spec builtin_types[]{
    {names::type::integer, 0, Builtin_type_id::integer},
    {names::type::list, 1, Builtin_type_id::list},
    {names::type::pair, 2, Builtin_type_id::pair},
    {names::type::stream, 1, Builtin_type_id::stream},
    {names::type::lazy_pair, 2, Builtin_type_id::lazy_pair},
    {names::type::lambda, 2, Builtin_type_id::lambda},
};

// -BUILTIN STRUCTORS-
// (I couldn't find a way to make clang-format format this better, sorry)

using Builtin_structor_spec =
    std::tuple<std::string_view, Builtin_type_id, std::vector<Type_template>,
               std::vector<Type_template>, Builtin_abstraction_id>;

const Builtin_structor_spec builtin_structors[] = {
    {
        names::structor::nil,
        Builtin_type_id::list,
        {},
        {},
        Builtin_abstraction_id::list_nil,
    },
    {
        names::structor::cons,
        Builtin_type_id::list,
        {Type_template_var{0},
         box(Concrete_type_template{Builtin_type_id::list,
                                    {Type_template_var{0}}})},
        {},
        Builtin_abstraction_id::list_cons,
    },
    {
        names::structor::pair,
        Builtin_type_id::pair,
        {Type_template_var{0}, Type_template_var{1}},
        {},
        Builtin_abstraction_id::pair_pair,
    },
    {
        names::structor::head,
        Builtin_type_id::stream,
        {},
        {Type_template_var{0}},
        Builtin_abstraction_id::stream_head,
    },
    {
        names::structor::tail,
        Builtin_type_id::stream,
        {},
        {box(Concrete_type_template{Builtin_type_id::stream,
                                    {Type_template_var{0}}})},
        Builtin_abstraction_id::stream_tail,
    },
    {
        names::structor::fst,
        Builtin_type_id::lazy_pair,
        {},
        {Type_template_var{0}},
        Builtin_abstraction_id::lazy_pair_fst,
    },
    {
        names::structor::snd,
        Builtin_type_id::lazy_pair,
        {},
        {Type_template_var{1}},
        Builtin_abstraction_id::lazy_pair_snd,
    },
    {
        names::structor::ap,
        Builtin_type_id::lambda,
        {Type_template_var{0}},
        {Type_template_var{1}},
        Builtin_abstraction_id::lambda_ap,
    },
};

Typing_context default_typing_context()
{
    Typing_context ctx;
    // Add builtin types
    for (auto& [name, n_params, id] : builtin_types) {
        ctx.add_type_prototype(name, n_params, id);
    }
    // Add builtin structors (the copying is intentional)
    for (auto [name, type_id, args, coargs, id] : builtin_structors) {
        ctx.add_structor(name, type_id, std::move(args), std::move(coargs), id);
    }
    return ctx;
}

} // namespace lammm