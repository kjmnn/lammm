#include "Parser.h"
#include <sstream>

using namespace lammm::names;

namespace lammm {

constexpr std::tuple<std::string_view, Abstraction_id, Parser::Arity_info,
                     Parser::Syntax_polarity>
    builtin_structors[]{
        {structor::nil,
         Builtin_abstraction_id::list_nil,
         {0, 0},
         Parser::Syntax_polarity::producer},
        {structor::cons,
         Builtin_abstraction_id::list_cons,
         {2, 0},
         Parser::Syntax_polarity::producer},
        {structor::pair,
         Builtin_abstraction_id::pair_pair,
         {2, 0},
         Parser::Syntax_polarity::producer},
        {structor::head,
         Builtin_abstraction_id::stream_head,
         {0, 1},
         Parser::Syntax_polarity::consumer},
        {structor::tail,
         Builtin_abstraction_id::stream_tail,
         {0, 1},
         Parser::Syntax_polarity::consumer},
        {structor::fst,
         Builtin_abstraction_id::lazy_pair_fst,
         {0, 1},
         Parser::Syntax_polarity::consumer},
        {structor::snd,
         Builtin_abstraction_id::lazy_pair_snd,
         {0, 1},
         Parser::Syntax_polarity::consumer},
        {structor::ap,
         Builtin_abstraction_id::lambda_ap,
         {1, 1},
         Parser::Syntax_polarity::consumer},
    };

Parser::Parser(Typing_context& ctx)
    : ctx(ctx)
{
    // Register builtin structor arities
    for (auto& [name, abstraction_id, arity, polarity] : builtin_structors) {
        structor_arity.emplace(abstraction_id, arity);
        switch (polarity) {
            case Syntax_polarity::producer:
                constructor_ids.emplace(name, abstraction_id);
                break;
            case Syntax_polarity::consumer:
                destructor_ids.emplace(name, abstraction_id);
                break;
            default:
                // Unreachable
                assert(false);
        }
    }
}

Producer Parser::parse_producer(std::istream& input)
{
    skip_whitespace(input);
    auto start_line = current_line;
    auto next = input.peek();
    if (std::isalpha(next)) {
        return box(parse_variable(input));
    } else if (std::isdigit(next) || next == symbol::minus) {
        return box(parse_value(input));
    }
    expect(input, symbol::open_paren, AST::producer, start_line);
    auto word = peek_word(input, 6);
    input.putback(symbol::open_paren); // unconsume open paren
    if (word == keyword::mu_p_ascii || word == keyword::mu_p_unicode) {
        return box(parse_mu_p(input));
    } else if (word == keyword::cocase) {
        return box(parse_cocase(input));
    } else {
        return box(parse_constructor(input));
    }
}

Consumer Parser::parse_consumer(std::istream& input)
{
    skip_whitespace(input);
    auto start_line = current_line;
    auto next = input.peek();
    if (std::isalpha(next)) {
        return box(parse_covariable(input));
    } else if (next == keyword::end[0]) {
        return box(parse_end(input));
    }
    expect(input, symbol::open_paren, AST::consumer, start_line);
    auto word = peek_word(input, 5);
    input.putback(symbol::open_paren); // unconsume open paren
    if (word == keyword::mu_c_ascii || word == keyword::mu_c_unicode) {
        return box(parse_mu_c(input));
    } else if (word == keyword::case_) {
        return box(parse_case(input));
    } else {
        return box(parse_destructor(input));
    }
}

Statement Parser::parse_statement(std::istream& input)
{
    skip_whitespace(input);
    auto start_line = current_line;
    if (input.peek() == symbol::open_square) {
        return box(parse_cut(input));
    }
    expect(input, symbol::open_paren, AST::statement, start_line);
    auto next = input.peek();
    switch (next) {
        case symbol::plus:
        case symbol::minus:
        case symbol::star:
        case symbol::slash:
        case symbol::modulo:
            input.putback(symbol::open_paren); // unconsume open paren
            return box(parse_arithmetic(input));
        default:
            if (std::isalpha(next)) {
                auto word = peek_word(input, 3);
                input.putback(symbol::open_paren); // unconsume open paren
                if (word == keyword::ifz) {
                    return box(parse_ifz(input));
                } else {
                    return box(parse_call(input));
                }
            } else {
                throw Unexpected_char_exception{current_line, start_line,
                                                std::string(AST::statement),
                                                next};
            }
    }
}

Definition Parser::parse_definition(std::istream& input)
{
    skip_whitespace(input);
    auto start_line = current_line;
    expect(input, symbol::open_paren, AST::definition, start_line);
    // Keyword
    auto keyword = read_word(input);
    if (keyword != keyword::def) {
        auto cause = keyword.empty() ? input.peek() : keyword[0];
        throw Unexpected_char_exception{current_line, start_line,
                                        std::string(AST::definition), cause};
    }
    // Name
    auto name = read_word(input);
    if (name.empty()) {
        throw Unexpected_char_exception{current_line, start_line,
                                        std::string(AST::definition),
                                        input.peek()};
    } else if (def_ids.contains(name)) {
        throw Parse_exception{current_line, start_line,
                              std::string(AST::definition),
                              std::format("Repeated definition of {}", name)};
    } else if (name == keyword::ifz) {
        // The only possible conflict, ifz is the only statement keyword
        throw Parse_exception{
            current_line, start_line, std::string(AST::definition),
            std::format("{} is a reserved name", keyword::ifz)};
    }
    auto def_id = Definition_id{n_defs++};
    def_ids.emplace(name, def_id);
    // Args & coargs
    auto arg_names = parse_list<std::string>(input, AST::parameter);
    auto coarg_names = parse_list<std::string>(input, AST::coparameter);
    auto arg_ids = std::vector<Var_id>{};
    arg_ids.reserve(arg_names.size());
    for (auto& arg_name : arg_names) {
        Var_id arg_id{n_vars_++};
        var_ctx[arg_name].push_back(arg_id);
        arg_ids.push_back(arg_id);
    }
    auto coarg_ids = std::vector<Covar_id>{};
    coarg_ids.reserve(coarg_names.size());
    for (auto& coarg_name : coarg_names) {
        Covar_id coarg_id{n_covars_++};
        covar_ctx[coarg_name].push_back(coarg_id);
        coarg_ids.push_back(coarg_id);
    }
    // Register arity info
    def_arity.emplace(def_id, Arity_info{arg_ids.size(), coarg_ids.size()});
    // Register abstraction in typing context
    auto abstraction_id =
        ctx.add_definition(name, arg_ids.size(), coarg_ids.size());
    // Body & cleanup
    auto body = parse_statement(input);
    for (auto& arg_name : arg_names) {
        var_ctx[arg_name].pop_back();
    }
    for (auto& coarg_name : coarg_names) {
        covar_ctx[coarg_name].pop_back();
    }
    expect(input, symbol::close_paren, AST::definition, start_line);
    return {
        .abstraction_id = abstraction_id,
        .definition_name = std::move(name),
        .arg_names = std::move(arg_names),
        .coarg_names = std::move(coarg_names),
        .arg_ids = std::move(arg_ids),
        .coarg_ids = std::move(coarg_ids),
        .body = std::move(body),
    };
}

Program Parser::parse_program(std::istream& input)
{
    Program program;
    skip_whitespace(input);
    while (!input.eof()) {
        if (input.peek() == symbol::open_square) {
            program.statements.emplace_back(box(parse_cut(input)));
            skip_whitespace(input);
            continue;
        }
        expect(input, symbol::open_paren, AST::def_or_stmt, current_line);
        auto word = peek_word(input, 3);
        input.putback(symbol::open_paren); // unconsume open paren
        if (word == keyword::def) {
            program.definitions.emplace_back(parse_definition(input));
        } else {
            program.statements.emplace_back(parse_statement(input));
        }
        skip_whitespace(input);
    }
    return program;
}

Variable_prod Parser::parse_variable(std::istream& input)
{
    std::string name = read_word(input);
    if (name.empty()) {
        throw Unexpected_char_exception{current_line, current_line,
                                        std::string(AST::variable),
                                        input.peek()};
    }
    if (var_ctx[name].empty()) {
        throw Unknown_name_exception{
            current_line, current_line, std::string(AST::variable),
            std::string(AST::variable), std::move(name)};
    }
    return {
        .var_id = var_ctx[name].back(),
        .var_name = std::move(name),
    };
}

Value_prod Parser::parse_value(std::istream& input)
{
    std::string literal = read_word(input);
    if (literal.empty()) {
        throw Unexpected_char_exception{current_line, current_line,
                                        std::string(AST::value), input.peek()};
    }
    std::size_t processed;
    std::int64_t value = std::stoll(literal, &processed);
    if (processed != literal.size()) {
        throw Parse_exception{
            current_line, current_line, std::string(AST::value),
            std::format("invalid integer literal: {}", literal)};
    }
    return {.value = value};
}

Mu_prod Parser::parse_mu_p(std::istream& input)
{
    skip_whitespace(input);
    auto start_line = current_line;
    expect(input, symbol::open_paren, AST::mu_p, start_line);
    // Keyword
    std::string keyword = read_word(input);
    if (keyword != keyword::mu_p_ascii && keyword != keyword::mu_p_unicode) {
        auto cause = keyword.empty() ? input.peek() : keyword[0];
        throw Unexpected_char_exception{current_line, start_line,
                                        std::string(AST::mu_p), cause};
    }
    // Coarg
    auto coarg_name = read_word(input);
    if (coarg_name.empty()) {
        throw Unexpected_char_exception{current_line, start_line,
                                        std::string(AST::mu_p), input.peek()};
    }
    Covar_id coarg_id{n_covars_++};
    covar_ctx[coarg_name].push_back(coarg_id);
    // Body & cleanup
    auto body = parse_statement(input);
    covar_ctx[coarg_name].pop_back();
    expect(input, symbol::close_paren, AST::mu_p, start_line);
    return {
        .coarg_id = coarg_id,
        .coarg_name = std::move(coarg_name),
        .body = std::move(body),
    };
}

Constructor_prod Parser::parse_constructor(std::istream& input)
{
    auto [structor_id, name, args, coargs] =
        parse_structor(input, Syntax_polarity::producer);
    return {
        .abstraction_id = structor_id,
        .constructor_name = std::move(name),
        .args = std::move(args),
        .coargs = std::move(coargs),
    };
}

Cocase_prod Parser::parse_cocase(std::istream& input)
{
    skip_whitespace(input);
    auto start_line = current_line;
    expect(input, symbol::open_paren, AST::cocase, start_line);
    auto keyword = read_word(input);
    if (keyword != keyword::cocase) {
        auto cause = keyword.empty() ? input.peek() : keyword[0];
        throw Unexpected_char_exception{current_line, start_line,
                                        std::string(AST::cocase), cause};
    }
    auto clauses = parse_clauses(input, Syntax_polarity::producer, start_line);
    expect(input, symbol::close_paren, AST::cocase, start_line);
    return {
        .clauses = std::move(clauses),
    };
}

Covariable_cons Parser::parse_covariable(std::istream& input)
{
    std::string name = read_word(input);
    if (name.empty()) {
        throw Unexpected_char_exception{current_line, current_line,
                                        std::string(AST::covariable),
                                        input.peek()};
    }
    if (covar_ctx[name].empty()) {
        throw Unknown_name_exception{
            current_line, current_line, std::string(AST::covariable),
            std::string(AST::covariable), std::move(name)};
    }
    return {
        .covar_id = covar_ctx[name].back(),
        .covar_name = std::move(name),
    };
}

Mu_cons Parser::parse_mu_c(std::istream& input)
{
    skip_whitespace(input);
    auto start_line = current_line;
    expect(input, symbol::open_paren, AST::mu_c, start_line);
    std::string keyword = read_word(input);
    if (keyword != keyword::mu_c_ascii && keyword != keyword::mu_c_unicode) {
        auto cause = keyword.empty() ? input.peek() : keyword[0];
        throw Unexpected_char_exception{current_line, start_line,
                                        std::string(AST::mu_c), cause};
    }
    skip_whitespace(input);
    auto arg_name = read_word(input);
    if (arg_name.empty()) {
        throw Unexpected_char_exception{current_line, start_line,
                                        std::string(AST::mu_c), input.peek()};
    }
    Var_id arg_id{n_vars_++};
    var_ctx[arg_name].push_back(arg_id);
    auto body = parse_statement(input);
    var_ctx[arg_name].pop_back();
    expect(input, symbol::close_paren, AST::mu_c, start_line);
    return {
        .arg_id = arg_id,
        .arg_name = std::move(arg_name),
        .body = std::move(body),
    };
}

Destructor_cons Parser::parse_destructor(std::istream& input)
{
    auto [structor_id, name, args, coargs] =
        parse_structor(input, Syntax_polarity::consumer);
    return {
        .abstraction_id = structor_id,
        .destructor_name = std::move(name),
        .args = std::move(args),
        .coargs = std::move(coargs),
    };
}

Case_cons Parser::parse_case(std::istream& input)
{
    skip_whitespace(input);
    auto start_line = current_line;
    expect(input, symbol::open_paren, AST::case_, start_line);
    auto keyword = read_word(input);
    if (keyword != keyword::case_) {
        auto cause = keyword.empty() ? input.peek() : keyword[0];
        throw Unexpected_char_exception{current_line, start_line,
                                        std::string(AST::case_), cause};
    }
    auto clauses = parse_clauses(input, Syntax_polarity::consumer, start_line);
    expect(input, symbol::close_paren, AST::case_, start_line);
    return {
        .clauses = std::move(clauses),
    };
}

End_cons Parser::parse_end(std::istream& input)
{
    skip_whitespace(input);
    auto keyword = read_word(input);
    if (keyword != keyword::end) {
        auto cause = keyword.empty() ? input.peek() : keyword[0];
        throw Unexpected_char_exception{current_line, current_line,
                                        std::string(AST::end), cause};
    }
    return {};
}

Arithmetic_stmt Parser::parse_arithmetic(std::istream& input)
{
    skip_whitespace(input);
    auto start_line = current_line;
    expect(input, symbol::open_paren, AST::arithmetic, start_line);
    auto op_symbol = input.get();
    Arithmetic_op op;
    switch (op_symbol) {
        case symbol::plus:
            op = Arithmetic_op::add;
            break;
        case symbol::minus:
            op = Arithmetic_op::sub;
            break;
        case symbol::star:
            op = Arithmetic_op::mul;
            break;
        case symbol::slash:
            op = Arithmetic_op::div;
            break;
        case symbol::modulo:
            op = Arithmetic_op::mod;
            break;
        default:
            throw Unexpected_char_exception{current_line, start_line,
                                            std::string(AST::arithmetic),
                                            op_symbol};
            break;
    }
    auto left = parse_producer(input);
    auto right = parse_producer(input);
    auto after = parse_consumer(input);
    expect(input, symbol::close_paren, AST::arithmetic, start_line);
    return {
        .op = op,
        .left = std::move(left),
        .right = std::move(right),
        .after = std::move(after),
    };
}

Ifz_stmt Parser::parse_ifz(std::istream& input)
{
    skip_whitespace(input);
    auto start_line = current_line;
    expect(input, symbol::open_paren, AST::ifz, start_line);
    auto keyword = read_word(input);
    if (keyword != keyword::ifz) {
        auto cause = keyword.empty() ? input.peek() : keyword[0];
        throw Unexpected_char_exception{current_line, start_line,
                                        std::string(AST::ifz), cause};
    }
    auto condition = parse_producer(input);
    auto if_zero = parse_statement(input);
    auto if_other = parse_statement(input);
    expect(input, symbol::close_paren, AST::ifz, start_line);
    return {
        .condition = std::move(condition),
        .if_zero = std::move(if_zero),
        .if_other = std::move(if_other),
    };
}

Cut_stmt Parser::parse_cut(std::istream& input)
{
    skip_whitespace(input);
    auto start_line = current_line;
    expect(input, symbol::open_square, AST::cut, start_line);
    auto producer = parse_producer(input);
    auto consumer = parse_consumer(input);
    expect(input, symbol::close_square, AST::cut, start_line);
    return {
        .producer = std::move(producer),
        .consumer = std::move(consumer),
    };
}

Call_stmt Parser::parse_call(std::istream& input)
{
    skip_whitespace(input);
    auto start_line = current_line;
    expect(input, symbol::open_paren, AST::call, start_line);
    auto name = read_word(input);
    if (name.empty()) {
        throw Unexpected_char_exception{current_line, start_line,
                                        std::string(AST::call), input.peek()};
    }
    if (!def_ids.contains(name)) {
        throw Unknown_name_exception{
            current_line, start_line, std::string(AST::call),
            std::string(AST::definition), std::move(name)};
    }
    auto def_id = def_ids.at(name);
    auto info = def_arity.at(def_id);
    auto args = parse_list<Producer>(input, AST::producer);
    auto coargs = parse_list<Consumer>(input, AST::consumer);
    if (args.size() != info.arity) {
        throw Arity_mismatch_exception{
            current_line,
            start_line,
            std::string(AST::call),
            std::move(name),
            Syntax_polarity::producer,
            info.arity,
            args.size(),
        };
    }
    if (coargs.size() != info.coarity) {
        throw Arity_mismatch_exception{
            current_line,
            start_line,
            std::string(AST::call),
            std::move(name),
            Syntax_polarity::consumer,
            info.coarity,
            coargs.size(),
        };
    }
    expect(input, symbol::close_paren, AST::call, start_line);
    return {
        .definition_id = def_id,
        .definition_name = std::move(name),
        .args = std::move(args),
        .coargs = std::move(coargs),
    };
}

Clause Parser::parse_case_clause(std::istream& input)
{
    return parse_clause(input, Syntax_polarity::consumer);
}

Clause Parser::parse_cocase_clause(std::istream& input)
{
    return parse_clause(input, Syntax_polarity::producer);
}

std::size_t Parser::n_vars()
{
    return n_vars_;
}

std::size_t Parser::n_covars()
{
    return n_covars_;
}

Clause Parser::parse_clause(std::istream& input, Syntax_polarity p)
{
    auto clause_kind =
        p == Syntax_polarity::producer ? AST::cocase_clause : AST::case_clause;
    auto structor_kind =
        p == Syntax_polarity::producer ? AST::destructor : AST::constructor;
    auto& structor_ids =
        p == Syntax_polarity::producer ? destructor_ids : constructor_ids;
    skip_whitespace(input);
    auto start_line = current_line;
    expect(input, symbol::open_paren, clause_kind, start_line);
    // Parse structor name
    auto structor_name = read_word(input);
    if (structor_name.empty()) {
        throw Unexpected_char_exception{current_line, start_line,
                                        std::string(clause_kind), input.peek()};
    }
    if (!structor_ids.contains(structor_name)) {
        throw Unknown_name_exception{
            current_line, start_line, std::string(clause_kind),
            std::string(structor_kind), std::move(structor_name)};
    }
    auto& structor_id = structor_ids.at(structor_name);
    auto& info = structor_arity.at(structor_id.id);
    // Args & coargs
    std::vector<std::string> arg_names;
    if (info.arity > 0) {
        arg_names = parse_list<std::string>(input, AST::parameter);
    }
    if (arg_names.size() != info.arity) {
        throw Arity_mismatch_exception{
            current_line,
            start_line,
            std::string(clause_kind),
            std::move(structor_name),
            Syntax_polarity::producer,
            info.arity,
            arg_names.size(),
        };
    }
    std::vector<std::string> coarg_names;
    if (info.coarity > 0) {
        coarg_names = parse_list<std::string>(input, AST::coparameter);
    }
    if (coarg_names.size() != info.coarity) {
        throw Arity_mismatch_exception{
            current_line,
            start_line,
            std::string(clause_kind),
            std::move(structor_name),
            Syntax_polarity::consumer,
            info.coarity,
            coarg_names.size(),
        };
    }
    auto arg_ids = std::vector<Var_id>{};
    arg_ids.reserve(arg_names.size());
    for (auto& arg_name : arg_names) {
        Var_id arg_id{n_vars_++};
        var_ctx[arg_name].push_back(arg_id);
        arg_ids.push_back(arg_id);
    }
    auto coarg_ids = std::vector<Covar_id>{};
    coarg_ids.reserve(coarg_names.size());
    for (auto& coarg_name : coarg_names) {
        Covar_id coarg_id{n_covars_++};
        covar_ctx[coarg_name].push_back(coarg_id);
        coarg_ids.push_back(coarg_id);
    }
    // Body & cleanup
    auto body = parse_statement(input);
    for (auto& arg_name : arg_names) {
        var_ctx[arg_name].pop_back();
    }
    for (auto& coarg_name : coarg_names) {
        covar_ctx[coarg_name].pop_back();
    }
    expect(input, symbol::close_paren, AST::clause, start_line);
    return {
        .abstraction_id = structor_id,
        .structor_name = std::move(structor_name),
        .arg_names = std::move(arg_names),
        .coarg_names = std::move(coarg_names),
        .arg_ids = std::move(arg_ids),
        .coarg_ids = std::move(coarg_ids),
        .body = std::move(body),
    };
}

std::tuple<Abstraction_id, std::string, std::vector<Producer>,
           std::vector<Consumer>>
Parser::parse_structor(std::istream& input, Syntax_polarity p)
{
    auto structor_kind =
        p == Syntax_polarity::producer ? AST::constructor : AST::destructor;
    auto& structor_ids =
        p == Syntax_polarity::producer ? constructor_ids : destructor_ids;
    skip_whitespace(input);
    auto start_line = current_line;
    expect(input, symbol::open_paren, structor_kind, start_line);
    // Parse structor name
    auto structor_name = read_word(input);
    if (structor_name.empty()) {
        throw Unexpected_char_exception{
            current_line, start_line, std::string(structor_kind), input.peek()};
    }
    if (!structor_ids.contains(structor_name)) {
        throw Unknown_name_exception{
            current_line, start_line, std::string(structor_kind),
            std::string(structor_kind), std::move(structor_name)};
    }
    auto structor_id = structor_ids.at(structor_name);
    // Args & coargs
    auto& info = structor_arity.at(structor_id.id);
    std::vector<Producer> args;
    if (info.arity > 0) {
        args = parse_list<Producer>(input, AST::argument);
    }
    if (args.size() != info.arity) {
        throw Arity_mismatch_exception{
            current_line,
            start_line,
            std::string(structor_kind),
            std::move(structor_name),
            Syntax_polarity::producer,
            info.arity,
            args.size(),
        };
    }
    std::vector<Consumer> coargs;
    if (info.coarity > 0) {
        coargs = parse_list<Consumer>(input, AST::coargument);
    }
    if (coargs.size() != info.coarity) {
        throw Arity_mismatch_exception{
            current_line,
            start_line,
            std::string(structor_kind),
            std::move(structor_name),
            Syntax_polarity::consumer,
            info.coarity,
            coargs.size(),
        };
    }
    expect(input, symbol::close_paren, structor_kind, start_line);
    return {structor_id, std::move(structor_name), std::move(args),
            std::move(coargs)};
}

std::vector<Clause> Parser::parse_clauses(std::istream& input,
                                          Syntax_polarity p,
                                          std::size_t start_line)
{
    auto clause_kind =
        p == Syntax_polarity::producer ? AST::cocase_clause : AST::case_clause;
    auto expression_kind =
        p == Syntax_polarity::producer ? AST::cocase : AST::case_;
    auto clauses = parse_list<Clause>(input, clause_kind, p);
    if (clauses.empty()) {
        throw Parse_exception{current_line, start_line,
                              std::string(expression_kind),
                              std::format("empty {} list", AST::clause)};
    }
    // Check that structor types match and the clause set is total
    auto expected_structors = ctx.structors_like(clauses[0].abstraction_id);
    for (auto& clause : clauses) {
        if (!expected_structors.contains(clause.abstraction_id)) {
            throw Parse_exception{
                current_line, start_line, std::string(expression_kind),
                std::format("Duplicate or mismatched structor: {}",
                            clause.structor_name)};
        }
        expected_structors.erase(clause.abstraction_id);
    }
    if (!expected_structors.empty()) {
        throw Parse_exception{current_line, start_line,
                              std::string(expression_kind),
                              "incomplete clause list"};
    }
    return clauses;
}

void Parser::skip_whitespace(std::istream& input)
{
    while (!input.eof() && isspace(input.peek())) {
        if (input.peek() == '\n') {
            ++current_line;
        }
        input.get();
    }
}

std::string Parser::read_word(std::istream& input, std::size_t max_len)
{
    skip_whitespace(input);
    std::ostringstream word;
    std::size_t len = 0;
    while (true) {
        if (len >= max_len) {
            return word.str();
        }
        switch (int c = input.peek()) {
            case symbol::open_paren:
            case symbol::close_paren:
            case symbol::open_square:
            case symbol::close_square:
                return word.str();
            default:
                if (isspace(c) || input.eof()) {
                    return word.str();
                } else {
                    word.put(input.get());
                }
        }
        ++len;
    }
}

std::string Parser::peek_word(std::istream& input, std::size_t max_len)
{
    auto word = read_word(input, max_len);
    for (auto it = word.crbegin(); it != word.crend(); ++it) {
        input.putback(*it);
    }
    return word;
}

void Parser::expect(std::istream& input, char expected,
                    std::string_view context, std::size_t start_line)
{
    auto c = input.get();
    if (c != expected) {
        throw Unexpected_char_exception(current_line, start_line,
                                        std::string(context), c);
    }
}

Parse_exception::Parse_exception(std::size_t cause_line,
                                 std::size_t context_line,
                                 std::string&& context,
                                 std::string&& explanation)
    : message_(std::format(
          "On line {}, while parsing a {} (starting on line {}): {}",
          cause_line, context, context_line, explanation)),
      cause_line(cause_line),
      context_line(context_line),
      context(std::move(context))
{
}

std::string Parse_exception::name() const
{
    return "Parse error";
}

std::string Parse_exception::message() const
{
    return message_;
}

Unexpected_char_exception::Unexpected_char_exception(std::size_t cause_line,
                                                     std::size_t context_line,
                                                     std::string&& context,
                                                     int unexpected_char)
    : Parse_exception(cause_line, context_line, std::move(context),
                      explain(unexpected_char)),
      unexpected_char(unexpected_char)
{
}

std::string Unexpected_char_exception::explain(int unexpected_char)
{
    if (unexpected_char == std::istream::traits_type::eof()) {
        return "unexpected end of input";
    } else {
        return std::format("unexpected '{}'", char(unexpected_char));
    }
}

Unknown_name_exception::Unknown_name_exception(std::size_t cause_line,
                                               std::size_t context_line,
                                               std::string&& context,
                                               std::string&& syntax_kind,
                                               std::string&& name)
    : Parse_exception(cause_line, context_line, std::move(context),
                      std::format("unknown {}: {}", syntax_kind, name)),
      syntax_kind(std::move(syntax_kind)),
      name(std::move(name))
{
}

Arity_mismatch_exception::Arity_mismatch_exception(
    std::size_t cause_line, std::size_t context_line, std::string&& context,
    std::string&& syntax_name, Parser::Syntax_polarity polarity,
    std::size_t expected, std::size_t actual)
    : Parse_exception(cause_line, context_line, std::move(context),
                      explain(polarity, syntax_name, expected, actual)),
      syntax_name(std::move(syntax_name)),
      polarity(polarity),
      expected(expected),
      actual(actual)
{
}

std::string Arity_mismatch_exception::explain(Parser::Syntax_polarity polarity,
                                              std::string& syntax_name,
                                              std::size_t expected,
                                              std::size_t actual)
{
    assert(polarity == Parser::Syntax_polarity::producer ||
           polarity == Parser::Syntax_polarity::consumer);
    return std::format("{} mismatch: {} expects {}, got {}",
                       polarity == Parser::Syntax_polarity::producer
                           ? misc::arity
                           : misc::coarity,
                       syntax_name, expected, actual);
}

} // namespace lammm