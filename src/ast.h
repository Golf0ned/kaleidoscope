#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

extern std::unordered_map<char, int> binopPrecedence;

llvm::Value *LogErrorV(const char *str);

class ExprAST {
    public:
        virtual ~ExprAST() = default;
        virtual llvm::Value *codegen() = 0;
};

class NumberExprAST : public ExprAST {
    public:
        NumberExprAST(double val);
        llvm::Value *codegen() override;

    private:
        double val;
};

class VariableExprAST : public ExprAST {
    public:
        VariableExprAST(const std::string &name);
        llvm::Value *codegen() override;
        const std::string getName();

    private:
        std::string name;
};

class BinaryExprAST : public ExprAST {
    public:
        BinaryExprAST(char op, std::unique_ptr<ExprAST> left,
                      std::unique_ptr<ExprAST> right);
        llvm::Value *codegen() override;

    private:
        char op;
        std::unique_ptr<ExprAST> left, right;
};

class UnaryExprAST : public ExprAST {
    public:
        UnaryExprAST(char op, std::unique_ptr<ExprAST> operand);
        llvm::Value *codegen() override;

    private:
        char op;
        std::unique_ptr<ExprAST> operand;
};

class CallExprAST : public ExprAST {
    public:
        CallExprAST(const std::string &callee,
                    std::vector<std::unique_ptr<ExprAST>> args);
        llvm::Value *codegen() override;

    private:
        std::string callee;
        std::vector<std::unique_ptr<ExprAST>> args;
};

class IfExprAST : public ExprAST {
    public:
        IfExprAST(std::unique_ptr<ExprAST> cond,
                  std::unique_ptr<ExprAST> tBranch,
                  std::unique_ptr<ExprAST> fBranch);
        llvm::Value *codegen() override;

    private:
        std::unique_ptr<ExprAST> cond;
        std::unique_ptr<ExprAST> tBranch;
        std::unique_ptr<ExprAST> fBranch;
};

class ForExprAST : public ExprAST {
    public:
        ForExprAST(std::string &varName, std::unique_ptr<ExprAST> start,
                   std::unique_ptr<ExprAST> end, std::unique_ptr<ExprAST> step,
                   std::unique_ptr<ExprAST> body);
        llvm::Value *codegen() override;

    private:
        std::string varName;
        std::unique_ptr<ExprAST> start, end, step, body;
};

using VarNamePair = std::pair<std::string, std::unique_ptr<ExprAST>>;

class VarExprAST : public ExprAST {
    public:
        VarExprAST(std::vector<VarNamePair> varNames,
                   std::unique_ptr<ExprAST> body);
        llvm::Value *codegen() override;

    private:
        std::vector<VarNamePair> varNames;
        std::unique_ptr<ExprAST> body;
};

class PrototypeAST {
    public:
        PrototypeAST(const std::string &name, std::vector<std::string> args,
                     bool isOperator = false, unsigned precedence = 0);
        const std::string &getName() const;
        const std::vector<std::string> &getArgs();
        bool isUnaryOp() const;
        bool isBinaryOp() const;
        char getOperatorName() const;
        unsigned getBinaryPrecedence() const;
        llvm::Function *codegen();

    private:
        std::string name;
        std::vector<std::string> args;
        bool isOperator;
        unsigned precedence;
};

class FunctionAST {
    public:
        FunctionAST(std::unique_ptr<PrototypeAST> proto,
                    std::unique_ptr<ExprAST> body);
        llvm::Function *codegen();

    private:
        std::unique_ptr<PrototypeAST> proto;
        std::unique_ptr<ExprAST> body;
};
