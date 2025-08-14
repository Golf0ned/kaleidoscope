#pragma once

#include <memory>
#include <string>
#include <vector>


class ExprAST {
    public:
        virtual ~ExprAST() = default;
};

class NumberExprAST : public ExprAST {
    public:
        NumberExprAST(double val);
    
    private:
        double val;
};

class VariableExprAST : public ExprAST {
    public:
        VariableExprAST(const std::string &name);
    
    private:
        std::string name;
};

class BinaryExprAST : public ExprAST {
    public:
        BinaryExprAST(char op, std::unique_ptr<ExprAST> left,
                      std::unique_ptr<ExprAST> right);
    
    private:
        char op;
        std::unique_ptr<ExprAST> left, right;
};

class CallExprAST : public ExprAST {
    public:
        CallExprAST(const std::string& callee,
                    std::vector<std::unique_ptr<ExprAST>> args);
    
    private:
        std::string callee;
        std::vector<std::unique_ptr<ExprAST>> args;
};

class PrototypeAST {
    public:
        PrototypeAST(const std::string &name,
                     std::vector<std::string> args);
        const std::string &getName() const;

    private:
        std::string name;
        std::vector<std::string> args;
};

class FunctionAST {
    public:
        FunctionAST(std::unique_ptr<PrototypeAST> proto,
                    std::unique_ptr<ExprAST> body);

    private:
        std::unique_ptr<PrototypeAST> proto;
        std::unique_ptr<ExprAST> body;
};

