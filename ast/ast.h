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

static std::unique_ptr<llvm::LLVMContext> Context;
static std::unique_ptr<llvm::IRBuilder<>> Builder;
static std::unique_ptr<llvm::Module> Module;
static std::map<std::string, llvm::Value *> NamedValues;

llvm::Value *LogErrorV(const char *str);

void initializeModule();
void dumpIR();

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

class CallExprAST : public ExprAST {
    public:
        CallExprAST(const std::string& callee,
                    std::vector<std::unique_ptr<ExprAST>> args);
        llvm::Value *codegen() override;
    
    private:
        std::string callee;
        std::vector<std::unique_ptr<ExprAST>> args;
};

class PrototypeAST {
    public:
        PrototypeAST(const std::string &name,
                     std::vector<std::string> args);
        const std::string &getName() const;
        const std::vector<std::string>& getArgs();
        llvm::Function *codegen();

    private:
        std::string name;
        std::vector<std::string> args;
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

