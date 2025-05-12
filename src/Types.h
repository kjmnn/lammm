#ifndef TYPES_H
#define TYPES_H

#include "Util.h"
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <variant>
#include <vector>

namespace lammm {

/// @brief IDs of builtin types; meant to be converted to Type_id
enum class Builtin_type_id : std::size_t
{
    integer,
    list,
    pair,
    stream,
    lazy_pair,
    lambda,
};

/// @brief IDs of types; used in e.g. Typing_context::get_type_prototype
struct Type_id
{
    Type_id(std::size_t id)
        : id(id)
    {
    }
    Type_id(Builtin_type_id id)
        : id(static_cast<std::size_t>(id))
    {
    }
    std::size_t id;
    bool operator==(const Type_id& other) const
    {
        return id == other.id;
    }
};

/// @brief IDs of builtin (con/de)structors and definitions; meant to be converted to Abstraction_id
enum class Builtin_abstraction_id : std::size_t
{
    // Structors
    list_nil,
    list_cons,
    pair_pair,
    stream_head,
    stream_tail,
    lazy_pair_fst,
    lazy_pair_snd,
    lambda_ap,
};

/// @brief IDs of (con/de)structors and definitions; used in Typing_context::get_abstraction
struct Abstraction_id
{
    constexpr Abstraction_id(std::size_t id)
        : id(id)
    {
    }
    constexpr Abstraction_id(Builtin_abstraction_id id)
        : id(static_cast<std::size_t>(id))
    {
    }

    std::size_t id;
    bool operator==(const Abstraction_id& other) const
    {
        return id == other.id;
    }
};

inline bool operator<(const Abstraction_id& a, const Abstraction_id& b)
{
    return a.id < b.id;
}

/// @brief Used to obtain immutable references to types from a Typing_context
struct Type_handle
{
    std::size_t id;
    bool operator==(const Type_handle& other) const
    {
        return id == other.id;
    }
};

inline bool operator<(const Type_handle& a, const Type_handle& b)
{
    return a.id < b.id;
}

/// @brief Owning type handle, allows for unification and other such modification
struct Strong_type_handle
{
    std::size_t id;
    operator Type_handle() const
    {
        return Type_handle{id};
    }
};

/// @brief A type variable - a hole that can be unified with any type
struct Type_var
{
    /// @brief ID (for display purposes only)
    std::size_t id;
};

/// @brief A type with structure
struct Concrete_type
{
    Type_id type_id;
    std::vector<Type_handle> params;
};

/// @brief Used for type prototypes as well as types of actual producers & consumers
using Type_instance = std::variant<Type_var, Concrete_type, Type_handle>;

struct Concrete_type_template;

struct Type_template_var
{
    std::size_t id;
};

/// @brief A type template, used when adding new (con/de)structors to specify
///        how their (co)arguments relate to the parameters of their result type
using Type_template =
    std::variant<Type_template_var, Box<Concrete_type_template>>;

/// @brief The Type_template equivalent of Concrete_type
struct Concrete_type_template
{
    Type_id type_id;
    std::vector<Type_template> params;
};

/// @brief A common base for Abstraction and Abstraction_instance
/// @tparam Handle Type of handles (strong/regular) to relevant types
template <typename Handle>
struct Abstraction_
{
    /// @brief The result type - absent for definitions, instances of which are statements
    std::optional<Handle> type;
    /// @brief The structor or definition name (e.g. Nil or Ap)
    std::string abstraction_name;
    /// @brief Types of arguments
    std::vector<Handle> args;
    /// @brief Types of coarguments
    std::vector<Handle> coargs;

    /// @brief Get abstraction arity
    /// @return arity (number of arguments)
    inline std::size_t arity() const
    {
        return args.size();
    }
    /// @brief Get abstraction coarity
    /// @return coarity (number of coarguments)
    inline std::size_t coarity() const
    {
        return coargs.size();
    }
};

/// @brief An abstraction - type signature of a constructor, destructor or definition
using Abstraction = Abstraction_<Type_handle>;
/// @brief Used to package results of abstraction instantiation
using Abstraction_instance = Abstraction_<Strong_type_handle>;

/// @brief The Mighty Typing Context. Encapsulates most things related to typing.
class Typing_context
{
  public:
    /// @brief Add a brand new concrete type
    /// @param name The new type's name
    /// @param n_params The new type's number of parameters
    /// @param id if set, assert that the type's ID will be equal to this
    /// @return The ID of the new type
    Type_id add_type_prototype(std::string_view name, std::size_t n_params,
                               std::optional<Type_id> id = std::nullopt);
    /// @brief Add a new constructor or destructor
    /// @param name The new constructor's name
    /// @param type_id The ID of the (con/de)structor's result type
    /// @param args Type templates of the structor's arguments
    /// @param coargs Type templates of the structor's coarguments
    /// @param id if set, assert that the structor's ID will be equal to this
    /// @return The ID of the new structor
    Abstraction_id add_structor(
        std::string_view name, Type_id type_id,
        std::vector<Type_template>&& args, std::vector<Type_template>&& coargs,
        std::optional<Abstraction_id> id = std::nullopt);
    /// @brief Add a definition's type signature
    /// @param name Definition name
    /// @param args Argument types
    /// @param coargs Coargument types
    /// @param id if set, assert that the defintion's ID will be equal to this
    /// @return The ID of the new definition
    Abstraction_id add_definition(
        std::string_view name, std::size_t arity, std::size_t coarity,
        std::optional<Abstraction_id> id = std::nullopt);
    /// @brief Get a type instance (e.g. for printing)
    /// @param ref Handle to the type instance
    /// @return Constant reference to the type instance
    const Type_instance& get_type_instance(Type_handle ref) const;
    /// @brief Get handle to type prototype
    /// @param id Type ID
    /// @return Regular handle to the type prototype
    Type_handle get_type_prototype(Type_id id) const;
    /// @brief Get strong handle to a primitive type prototype
    ///        (which has no constructors, but also no variables to mangle during unification)
    /// @param id Type ID - MUST be of a primitive type (that is, integer)
    /// @return Strong handle to the primitive type prototype
    Strong_type_handle get_primitive_prototype(Type_id id) const;
    /// @brief Get reference to an abstraction
    /// @param id The abstraction's ID
    /// @return const reference to the abstraction with ID @p id
    const Abstraction& get_abstraction(Abstraction_id id) const;
    /// @brief Get abstraction prototype (USE WITH CAUTION)
    /// @param id The abstraction's ID
    /// @return Abstraction instance linked directly to the abstraction's prototype signature
    const Abstraction_instance get_abstraction_prototype(
        Abstraction_id id) const;
    /// @brief Get name of a type (e.g. "Integer" or "List")
    /// @param id The type's ID
    /// @return const reference to the type's name
    const std::string& get_type_name(Type_id id) const;
    /// @brief Get all structors of the same type as @p id
    /// @param id Abstraction ID of the structor
    /// @return set of IDs of structors with the same type as the one with ID @p id
    const std::set<Abstraction_id>& structors_like(Abstraction_id id) const;
    /// @brief Instantiate an abstraction, transitively cloning all relevant types
    /// @param abstraction_id ID of the abstraction to instantiate
    /// @return an Abstraction_instance, which contains strong type handles
    Abstraction_instance instantiate(Abstraction_id abstraction_id);
    /// @brief Create a new type variable
    /// @return Strong handle to the new variable
    Strong_type_handle fresh_type_variable();
    /// @brief Unify two types. Unification either fails, throwing an exception,
    ///        or succeeds, in which case the types are unified in place, becoming the same type.
    /// @param a Handle to the first type
    /// @param b Handle to the second type
    /// @return true iff @p a occurs in @p b
    void unify(Strong_type_handle a, Strong_type_handle b);

  private:
    /// @brief Handles of free instances of types, to be cloned before use
    std::vector<Type_handle> type_prototypes = {};
    /// @brief Type names, for printing types
    std::vector<std::string> type_names = {};
    /// @brief Type structors for checking (co)case totality
    std::map<Type_handle, std::set<Abstraction_id>> type_structors = {};
    /// @brief Constructors, destructors & definitions
    std::vector<Abstraction> abstractions = {};
    /// @brief Type instances - includes both prototypes
    /// and types of actual producers, consumers & statements
    std::vector<Type_instance> types = {};

    /// @brief Recursive helper for unification
    /// @param a First type to unify
    /// @param b Second type to unify
    /// @throw @p Unification_exception if unification fails
    void unify_rec(Type_handle a, Type_handle b);
    /// @brief Check if a type variable occurs in another type
    /// @param a Handle to type variable to check for
    /// @param b Handle to type to check in (usually a concrete type)
    /// @return true iff @p a occurs in @p b
    bool occurs(Type_handle a, Type_handle b);
    /// @brief Clone types and the types they depend on, transitively
    /// @param types Types to clone
    /// @return Strong handles to clones of @p types, in the same order
    std::vector<Strong_type_handle> clone_types(
        std::vector<Type_handle>&& types);
    /// @brief Recursive helper for cloning types
    /// @param type The type to clone
    /// @param map mapping from original to clone type handles
    /// @return Type handle of the clone of @p type
    Strong_type_handle clone_type_rec(
        Type_handle type, std::map<std::size_t, Strong_type_handle>& map);
    /// @brief Instantiate a type template, replacing variables with types from @p params
    /// @param template_ Type template
    /// @param params Parameters to replace variables with
    /// @return Strong handle to the instantiated type
    Type_handle instantiate_type_template(
        const Type_template& template_, const std::vector<Type_handle>& params);
    /// @brief Dereference possible reference and compress the reference chain
    /// @param type Type to dereference
    /// @return If @p type is a reference,
    ///         the type at the end of the reference chain, otherwise @p type
    Type_handle try_dereference(Type_handle type);
    /// @brief Dereference possible reference (without compressing the reference chain)
    /// @param type Type to dereference
    /// @return If @p type is a reference,
    ///         the type at the end of the reference chain, otherwise @p type
    Type_handle try_dereference(Type_handle type) const;
};

/// @brief Unification fail exception
class Unification_exception : public Exception
{
  public:
    enum class Kind
    {
        occurs,
        mismatch,
    };

    Unification_exception(Kind kind, const Typing_context& ctx, Type_handle a,
                          Type_handle b)
        : kind{kind}, ctx{ctx}, a{a}, b{b}
    {
    }
    std::string name() const override;
    std::string message() const override;

  private:
    Kind kind;
    const Typing_context& ctx;
    Type_handle a;
    Type_handle b;
};

/// @brief Create a new instance of the default typing context
/// @return The default typing context, including all builtin types and structors
Typing_context default_typing_context();

} // namespace lammm

#endif