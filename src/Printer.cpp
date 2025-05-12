#include "Printer.h"
#include <format>

using namespace lammm::names;

namespace lammm {

void Printer::operator()(const Producer& prod)
{
    std::visit(*this, prod);
}

void Printer::operator()(const Consumer& cons)
{
    std::visit(*this, cons);
}

void Printer::operator()(const Statement& stmt)
{
    std::visit(*this, stmt);
}

void Printer::operator()(const Program& program)
{
    for (auto&& definition : program.definitions) {
        (*this)(definition);
        stream << std::endl;
    }
    for (auto&& statement : program.statements) {
        (*this)(statement);
        stream << std::endl;
    }
}

void Printer::operator()(const Definition& definition)
{
    stream << symbol::open_paren << keyword::def << symbol::space
           << definition.definition_name << symbol::space;
    print_list(definition.arg_names);
    stream << symbol::space;
    print_list(definition.coarg_names);
    stream << symbol::space;
    (*this)(definition.body);
    stream << symbol::close_paren;
}

void Printer::operator()(const Clause& clause)
{
    stream << symbol::open_paren << clause.structor_name;
    if (!clause.arg_names.empty()) {
        stream << symbol::space;
        print_list(clause.arg_names);
    }
    if (!clause.coarg_names.empty()) {
        stream << symbol::space;
        print_list(clause.coarg_names);
    }
    stream << symbol::space;
    (*this)(clause.body);
    stream << symbol::close_paren;
}

void Printer::operator()(Arithmetic_op op)
{
    switch (op) {
        case Arithmetic_op::add:
            stream << symbol::plus;
            break;
        case Arithmetic_op::sub:
            stream << symbol::minus;
            break;
        case Arithmetic_op::mul:
            stream << symbol::star;
            break;
        case Arithmetic_op::div:
            stream << symbol::slash;
            break;
        case Arithmetic_op::mod:
            stream << symbol::modulo;
            break;
    }
}

void Printer::operator()(const Box<Variable_prod>& prod)
{
    stream << prod->var_name;
    print_type_maybe(prod);
}

void Printer::operator()(const Box<Value_prod>& prod)
{
    stream << prod->value;
    print_type_maybe(prod);
}

void Printer::operator()(const Box<Mu_prod>& prod)
{
    stream << symbol::open_paren
           << (options.ascii ? keyword::mu_p_ascii : keyword::mu_p_unicode)
           << symbol::space << prod->coarg_name << symbol::space;
    (*this)(prod->body);
    stream << symbol::close_paren;
    print_type_maybe(prod);
}

void Printer::operator()(const Box<Constructor_prod>& prod)
{
    stream << symbol::open_paren << prod->constructor_name;
    if (!prod->args.empty()) {
        stream << symbol::space;
        print_list(prod->args);
    }
    if (!prod->coargs.empty()) {
        stream << symbol::space;
        print_list(prod->coargs);
    }
    stream << symbol::close_paren;
    print_type_maybe(prod);
}

void Printer::operator()(const Box<Cocase_prod>& prod)
{
    stream << symbol::open_paren << keyword::cocase << symbol::space;
    print_list(prod->clauses);
    stream << symbol::close_paren;
    print_type_maybe(prod);
}

void Printer::operator()(const Box<Covariable_cons>& cons)
{
    stream << cons->covar_name;
    print_type_maybe(cons);
}

void Printer::operator()(const Box<Mu_cons>& cons)
{
    stream << symbol::open_paren
           << (options.ascii ? keyword::mu_c_ascii : keyword::mu_c_unicode)
           << symbol::space << cons->arg_name << symbol::space;
    (*this)(cons->body);
    stream << symbol::close_paren;
    print_type_maybe(cons);
}

void Printer::operator()(const Box<Destructor_cons>& cons)
{
    stream << symbol::open_paren << cons->destructor_name;
    if (!cons->args.empty()) {
        stream << symbol::space;
        print_list(cons->args);
    }
    if (!cons->coargs.empty()) {
        stream << symbol::space;
        print_list(cons->coargs);
    }
    stream << symbol::close_paren;
    print_type_maybe(cons);
}

void Printer::operator()(const Box<Case_cons>& cons)
{
    stream << symbol::open_paren << keyword::case_ << symbol::space;
    print_list(cons->clauses);
    stream << symbol::close_paren;
    print_type_maybe(cons);
}

void Printer::operator()(const Box<End_cons>& cons)
{
    stream << keyword::end;
    print_type_maybe(cons);
}

void Printer::operator()(const Box<Arithmetic_stmt>& stmt)
{
    stream << symbol::open_paren;
    (*this)(stmt->op);
    stream << symbol::space;
    (*this)(stmt->left);
    stream << symbol::space;
    (*this)(stmt->right);
    stream << symbol::space;
    (*this)(stmt->after);
    stream << symbol::close_paren;
}

void Printer::operator()(const Box<Ifz_stmt>& stmt)
{
    stream << symbol::open_paren << keyword::ifz << symbol::space;
    (*this)(stmt->condition);
    stream << symbol::space;
    (*this)(stmt->if_zero);
    stream << symbol::space;
    (*this)(stmt->if_other);
    stream << symbol::close_paren;
}

void Printer::operator()(const Box<Cut_stmt>& stmt)
{
    stream << symbol::open_square;
    (*this)(stmt->producer);
    stream << symbol::space;
    (*this)(stmt->consumer);
    stream << symbol::close_square;
}

void Printer::operator()(const Box<Call_stmt>& stmt)
{
    stream << symbol::open_paren << stmt->definition_name << symbol::space;
    print_list(stmt->args);
    stream << symbol::space;
    print_list(stmt->coargs);
    stream << symbol::close_paren;
}

void Printer::operator()(const std::string& str)
{
    stream << str;
}

void Printer::operator()(const Type_handle& type)
{
    if (typing_ctx) {
        auto& type_instance = typing_ctx->get_type_instance(type);
        std::visit(*this, type_instance);
    } else {
        stream << std::format("<UNKNOWN TYPE: {}>", type.id);
    }
}

void Printer::operator()(const Type_var& type)
{
    stream << symbol::question << type.id;
}

void Printer::operator()(const Concrete_type& type)
{
    if (!type.params.empty()) {
        stream << symbol::open_paren;
    }
    stream << typing_ctx->get_type_name(type.type_id);
    for (auto& param : type.params) {
        stream << symbol::space;
        (*this)(param);
    }
    if (!type.params.empty()) {
        stream << symbol::close_paren;
    }
}

} // namespace lammm