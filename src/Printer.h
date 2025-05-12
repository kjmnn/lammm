#ifndef PRINTER_H
#define PRINTER_H

#include "AST.h"
#include "Types.h"
#include "Util.h"
#include "names/Names.h"
#include <iostream>

namespace lammm {

/// @brief Print options
struct Print_options
{
    /// @brief If true, use ASCII replacements for non-ASCII characters
    bool ascii = true;
    /// @brief If true, print types of all typed syntax elements
    bool print_types = false;
};

/// @brief A simple recursive printer for AST elements and types
class Printer
{
  public:
    Printer(Print_options&& options, std::ostream& stream,
            const Typing_context* typing_ctx)
        : options{std::move(options)}, stream{stream}, typing_ctx(typing_ctx)
    {
    }

    void operator()(const Producer& prod);
    void operator()(const Consumer& cons);
    void operator()(const Statement& stmt);
    void operator()(const Program& program);
    void operator()(const Definition& definition);
    void operator()(const Clause& clause);
    void operator()(Arithmetic_op op);
    void operator()(const Box<Variable_prod>& prod);
    void operator()(const Box<Value_prod>& prod);
    void operator()(const Box<Mu_prod>& prod);
    void operator()(const Box<Constructor_prod>& prod);
    void operator()(const Box<Cocase_prod>& prod);
    void operator()(const Box<Covariable_cons>& cons);
    void operator()(const Box<Mu_cons>& cons);
    void operator()(const Box<Destructor_cons>& cons);
    void operator()(const Box<Case_cons>& cons);
    void operator()(const Box<End_cons>& cons);
    void operator()(const Box<Arithmetic_stmt>& stmt);
    void operator()(const Box<Ifz_stmt>& stmt);
    void operator()(const Box<Cut_stmt>& stmt);
    void operator()(const Box<Call_stmt>& stmt);
    void operator()(const std::string& str);
    void operator()(const Type_handle& type);
    void operator()(const Type_var& type);
    void operator()(const Concrete_type& type);

    /// @brief Print a list of printable elements
    /// @tparam T Type of the elements
    /// @param list The list of elements to print
    template <typename T>
    void print_list(const std::vector<T>& list);

    /// @brief Print the type of a printable element if it has one and @p options.print_types is set
    /// @tparam T Type of the element
    /// @param typed_printable The element to print
    template <typename T>
    void print_type_maybe(const T& typed_printable);

    /// @brief Print a printable element via pointer
    /// @tparam T Type of the element
    /// @param printable The element to print
    template <typename T>
    void operator()(const T* printable);

  private:
    /// @brief Print options
    Print_options options;
    /// @brief Output stream
    std::ostream& stream;
    /// @brief Typing context pointer for printing types
    const Typing_context* typing_ctx;
};

template <typename T>
void Printer::print_list(const std::vector<T>& list)
{
    bool first = true;
    stream << names::symbol::open_paren;
    for (auto&& elem : list) {
        if (!first) {
            stream << names::symbol::space;
        }
        first = false;
        (*this)(elem);
    }
    stream << names::symbol::close_paren;
}

template <typename T>
inline void Printer::print_type_maybe(const T& typed_printable)
{
    if (options.print_types && typed_printable->type.has_value()) {
        stream << names::symbol::colon << names::symbol::space;
        (*this)(typed_printable->type.value());
    }
}

template <typename T>
inline void Printer::operator()(const T* printable)
{
    if (printable) {
        (*this)(*printable);
    } else {
        stream << "<UNKNOWN ELEMENT>";
    }
}

/// @brief Construct a printer and print the @p printable
/// @tparam T Type of the thing to print
/// @param printable The thing to print
/// @param options Print options
/// @param stream Target output stream
/// @param typing_ctx Optional typing context for printing types
template <typename T>
void print(const T& printable, Print_options&& options = {},
           std::ostream& stream = std::cout,
           const Typing_context* typing_ctx = nullptr)
{
    Printer printer{std::move(options), stream, typing_ctx};
    printer(printable);
}

} // namespace lammm

#endif