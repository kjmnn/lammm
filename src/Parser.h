#ifndef PARSER_H
#define PARSER_H
#include "AST.h"
#include "Types.h"
#include "Util.h"
#include "names/Names.h"
#include <cassert>
#include <format>
#include <iostream>
#include <map>
#include <set>
#include <tuple>
#include <vector>

namespace lammm {

/// @brief A relatively straightforward recursive descent parser
///        All parse methods should be able to handle leading whitespace
class Parser
{
  public:
    /// @brief Stores information about (co)arity of a structor or definition
    struct Arity_info
    {
        std::size_t arity;
        std::size_t coarity;
    };
    /// @brief Tag enum to facilitate code reuse between parse methods of dual syntax elements
    enum class Syntax_polarity
    {
        producer,
        consumer,
        none,
    };

    Parser(Typing_context& ctx);

    Producer parse_producer(std::istream& input);
    Consumer parse_consumer(std::istream& input);
    Statement parse_statement(std::istream& input);
    Definition parse_definition(std::istream& input);
    Program parse_program(std::istream& input);

    Variable_prod parse_variable(std::istream& input);
    Value_prod parse_value(std::istream& input);
    Mu_prod parse_mu_p(std::istream& input);
    Constructor_prod parse_constructor(std::istream& input);
    Cocase_prod parse_cocase(std::istream& input);
    Covariable_cons parse_covariable(std::istream& input);
    Mu_cons parse_mu_c(std::istream& input);
    Destructor_cons parse_destructor(std::istream& input);
    Case_cons parse_case(std::istream& input);
    End_cons parse_end(std::istream& input);
    Arithmetic_stmt parse_arithmetic(std::istream& input);
    Ifz_stmt parse_ifz(std::istream& input);
    Cut_stmt parse_cut(std::istream& input);
    Call_stmt parse_call(std::istream& input);

    Clause parse_case_clause(std::istream& input);
    Clause parse_cocase_clause(std::istream& input);

    /// @brief Get current variable count
    /// @return No. of variables encountered so far
    std::size_t n_vars();
    /// @brief Get current covariable count
    /// @return No. of covariables encountered so far
    std::size_t n_covars();

  private:
    Typing_context& ctx;
    std::size_t current_line = 1;
    /// @brief No. of variables encountered so far (used to generate IDs)
    std::size_t n_vars_ = 0;
    /// @brief No. of covariables encountered so far (used to generate IDs)
    std::size_t n_covars_ = 0;
    /// @brief No. of definitions encountered so far (used to generate IDs)
    std::size_t n_defs = 0;
    /// @brief Maps variable names to IDs in the current context
    std::map<std::string, std::vector<Var_id>> var_ctx;
    /// @brief Maps covariable names to IDs in the current context
    std::map<std::string, std::vector<Covar_id>> covar_ctx;
    /// @brief Maps definition names to IDs
    std::map<std::string, Definition_id> def_ids;
    /// @brief Maps constructor names to IDs
    std::map<std::string, Abstraction_id> constructor_ids;
    /// @brief Maps destructor names to IDs
    std::map<std::string, Abstraction_id> destructor_ids;
    /// @brief Maps structor IDs to (co)arity info
    std::map<Abstraction_id, Arity_info> structor_arity;
    /// @brief Maps definition IDs to (co)arity info
    std::map<Definition_id, Arity_info> def_arity;

    /// @brief Common implementation for case and cocase clauses;
    ///        Handles (con/de)structor lookup correctly
    /// @param input Input stream
    /// @param p @p Syntax_polarity::producer for cocase clauses,
    ///          @p Syntax_polarity::consumer for case clauses
    /// @return A clause containing only (de/con)structors
    Clause parse_clause(std::istream& input, Syntax_polarity p);
    /// @brief Common implementation for parsing (con/de)structors
    /// @param input Input stream
    /// @param p @p Syntax_polarity::producer for constructors,
    ///          @p Syntax_polarity::consumer for destructors
    /// @return Structor ID, name & arguments
    std::tuple<Abstraction_id, std::string, std::vector<Producer>,
               std::vector<Consumer>>
    parse_structor(std::istream& input, Syntax_polarity p);
    /// @brief Parse a set of (co)case clauses and check that they match and are total
    /// @param input Input stream
    /// @param p @p Syntax_polarity::producer for cocase clauses,
    ///          @p Syntax_polarity::consumer for case clauses
    /// @param start_line Starting line of the parent expression (for error reporting)
    /// @return A vector of clauses (guaranteed to be total, non-repeating & of correct polarity)
    std::vector<Clause> parse_clauses(std::istream& input, Syntax_polarity p,
                                      std::size_t start_line);

    /// @brief Skip whitespace, incrementing @p current_line on newlines
    /// @param input Input stream
    void skip_whitespace(std::istream& input);
    /// @brief Read until whitespace, EOF or a delimiter - one of @p ()[]
    ///        - or @p max_len characters are read
    /// @param input Input stream
    /// @param max_len Maximum length to read
    /// @return The read sequence of characters
    std::string read_word(std::istream& input, std::size_t max_len = -1);
    /// @brief Like @p read_word but puts the read characters back after
    /// @param input Input stream
    /// @param max_len Maximum length to peek
    /// @return The peeked sequence of characters
    std::string peek_word(std::istream& input, std::size_t max_len);
    /// @brief Consume the next character in @p input and check that it equals @p expected
    /// @param input Input stream
    /// @param expected Expected character
    /// @param context Name of syntax element being parsed
    /// @param start_line Starting line of syntax element being parsed
    /// @throw @p Unexpected_char_exception if the next character is different (incl. EOF)
    void expect(std::istream& input, char expected, std::string_view context,
                std::size_t start_line);

    /// @brief Parse a list of @p T
    /// @tparam T Parsed list element type
    /// @param input Input stream
    /// @param syntax_kind Name of syntax element the list consists of
    /// @param p Polarity; relevant for clauses, otherwise ignored
    /// @return List of parsed elements
    template <typename T>
    std::vector<T> parse_list(std::istream& input, std::string_view syntax_kind,
                              Syntax_polarity p = Syntax_polarity::none);
    /// @brief Parse a @p T ; meant for partial specialisation, to be used in @p parse_list<T>
    /// @tparam T Parsed element type
    /// @param input Input stream
    /// @param p Polarity; relevant for clauses, otherwise ignored
    /// @return Parsed @p T
    template <typename T>
    T parse(std::istream& input, Syntax_polarity p);
};

/// @brief Base class for parser exceptions; also used for one-off errors
struct Parse_exception : public Exception
{
    Parse_exception(std::size_t cause_line, std::size_t context_line,
                    std::string&& context, std::string&& explanation);
    std::string name() const override;
    std::string message() const override;

    const std::string message_;
    const std::size_t cause_line;
    const std::size_t context_line;
    const std::string context;
};

/// @brief Thrown when the parser encounters an unexpected character
struct Unexpected_char_exception : public Parse_exception
{
    Unexpected_char_exception(std::size_t cause_line, std::size_t context_line,
                              std::string&& context, int unexpected_char);

    const int unexpected_char;

  private:
    /// @brief Create an explanation message for unexpected character or EOF
    /// @param unexpected_char The unexpected characte
    /// @return "unexpected ..." message
    static std::string explain(int unexpected_char);
};

/// @brief Thrown when an undefined (co)variable, structor or definition is encountered
struct Unknown_name_exception : public Parse_exception
{
    Unknown_name_exception(std::size_t cause_line, std::size_t context_line,
                           std::string&& context, std::string&& syntax_kind,
                           std::string&& name);

    const std::string syntax_kind;
    const std::string name;
};

/// @brief Thrown when a (con/de)structor or definition is passed the wrong number of (co)arguments
struct Arity_mismatch_exception : public Parse_exception
{
    Arity_mismatch_exception(std::size_t cause_line, std::size_t context_line,
                             std::string&& context, std::string&& syntax_name,
                             Parser::Syntax_polarity polarity,
                             std::size_t expected, std::size_t actual);

    const std::string syntax_name;
    const Parser::Syntax_polarity polarity;
    const std::size_t expected;
    const std::size_t actual;

  private:
    /// @brief Create an explanation message for arity mismatch
    /// @param polarity @p Syntax_polarity::producer for arity, @p Syntax_polarity::consumer for coarity
    /// @param expected expected no. of arguments
    /// @param actual actual no. of arguments
    /// @return "arity mismatch..." message
    static std::string explain(Parser::Syntax_polarity polarity,
                               std::string& syntax_name, std::size_t expected,
                               std::size_t actual);
};

template <typename T>
std::vector<T> Parser::parse_list(std::istream& input,
                                  std::string_view syntax_kind,
                                  Syntax_polarity p)
{
    skip_whitespace(input);
    auto start_line = current_line;
    // not using expect(...) because the context is not a constant
    auto c = input.get();
    if (c != names::symbol::open_paren) {
        throw Unexpected_char_exception(current_line, start_line,
                                        std::format("{} list", syntax_kind), c);
    }
    std::vector<T> result;
    while (true) {
        skip_whitespace(input);
        if (input.peek() == names::symbol::close_paren) {
            input.get();
            break;
        } else if (input.eof()) {
            throw Unexpected_char_exception(current_line, start_line,
                                            std::format("{} list", syntax_kind),
                                            input.peek());
        }
        result.emplace_back(parse<T>(input, p));
    }
    return result;
}

template <>
inline Producer Parser::parse<Producer>(std::istream& input, Syntax_polarity)
{
    return parse_producer(input);
}
template <>
inline Consumer Parser::parse<Consumer>(std::istream& input, Syntax_polarity)
{
    return parse_consumer(input);
}
template <>
inline Clause Parser::parse<Clause>(std::istream& input, Syntax_polarity p)
{
    assert(p == Syntax_polarity::producer || p == Syntax_polarity::consumer);
    return parse_clause(input, p);
}
template <>
inline std::string Parser::parse<std::string>(std::istream& input,
                                              Syntax_polarity)
{
    return read_word(input);
}

} // namespace lammm

#endif