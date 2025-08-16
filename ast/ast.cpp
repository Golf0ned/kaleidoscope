#include "ast.h"


NumberExprAST::NumberExprAST(double val) : val(val) {}

VariableExprAST::VariableExprAST(const std::string &name)
    : name(name) {}

BinaryExprAST::BinaryExprAST(char op, std::unique_ptr<ExprAST> left,
                             std::unique_ptr<ExprAST> right)
    : op(op), left(std::move(left)), right(std::move(right)) {}

CallExprAST::CallExprAST(const std::string& callee,
                         std::vector<std::unique_ptr<ExprAST>> args)
    : callee(callee), args(std::move(args)) {}

PrototypeAST::PrototypeAST(const std::string &name,
                           std::vector<std::string> args)
    : name(name), args(args) {}

const std::string &PrototypeAST::getName() const {
    return name;
}

FunctionAST::FunctionAST(std::unique_ptr<PrototypeAST> proto,
                         std::unique_ptr<ExprAST> body)
    : proto(std::move(proto)), body(std::move(body)) {}
