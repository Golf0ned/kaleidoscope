#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"

#include "ast.h"
#include "llvm.h"

llvm::Value *LogErrorV(const char *str) {
    fprintf(stderr, "Error: %s\n", str);
    return nullptr;
}

NumberExprAST::NumberExprAST(double val) : val(val) {}

llvm::Value *NumberExprAST::codegen() {
    return llvm::ConstantFP::get(*Context, llvm::APFloat(val));
}

VariableExprAST::VariableExprAST(const std::string &name) : name(name) {}

llvm::Value *VariableExprAST::codegen() {
    llvm::Value *v = NamedValues[name];
    if (!v)
        LogErrorV("Unknown variable name");
    return v;
}

BinaryExprAST::BinaryExprAST(char op, std::unique_ptr<ExprAST> left,
                             std::unique_ptr<ExprAST> right)
    : op(op), left(std::move(left)), right(std::move(right)) {}

llvm::Value *BinaryExprAST::codegen() {
    llvm::Value *l = left->codegen();
    llvm::Value *r = right->codegen();
    if (!l || !r)
        return nullptr;

    switch (op) {
        case '+':
            return Builder->CreateFAdd(l, r, "addtmp");
        case '-':
            return Builder->CreateFSub(l, r, "subtmp");
        case '*':
            return Builder->CreateFMul(l, r, "multmp");
        case '<':
            l = Builder->CreateFCmpULT(l, r, "cmptmp");
            return Builder->CreateUIToFP(l, llvm::Type::getDoubleTy(*Context),
                                         "booltmp");
        default:
            return LogErrorV("invalid binary operator");
    }
}

CallExprAST::CallExprAST(const std::string &callee,
                         std::vector<std::unique_ptr<ExprAST>> args)
    : callee(callee), args(std::move(args)) {}

llvm::Value *CallExprAST::codegen() {
    llvm::Function *calleeF = Module->getFunction(callee);
    if (!calleeF)
        return LogErrorV("unknown function referenced");

    if (calleeF->arg_size() != args.size())
        return LogErrorV("incorrect # args passed");

    std::vector<llvm::Value *> argsV;
    for (auto &arg : args) {
        argsV.push_back(arg->codegen());
        if (!argsV.back())
            return nullptr;
    }

    return Builder->CreateCall(calleeF, argsV, "calltmp");
}

PrototypeAST::PrototypeAST(const std::string &name,
                           std::vector<std::string> args)
    : name(name), args(args) {}

const std::string &PrototypeAST::getName() const { return name; }

const std::vector<std::string> &PrototypeAST::getArgs() { return args; }

llvm::Function *PrototypeAST::codegen() {
    std::vector<llvm::Type *> doubles(args.size(),
                                      llvm::Type::getDoubleTy(*Context));
    llvm::FunctionType *ft = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*Context), doubles, false);

    llvm::Function *f = llvm::Function::Create(
        ft, llvm::Function::ExternalLinkage, name, Module.get());

    unsigned i = 0;
    for (auto &arg : f->args())
        arg.setName(args[i++]);

    return f;
}

FunctionAST::FunctionAST(std::unique_ptr<PrototypeAST> proto,
                         std::unique_ptr<ExprAST> body)
    : proto(std::move(proto)), body(std::move(body)) {}

llvm::Function *FunctionAST::codegen() {
    llvm::Function *f = Module->getFunction(proto->getName());

    if (!f)
        f = proto->codegen();

    if (!f)
        return nullptr;

    if (f->arg_size() != proto->getArgs().size())
        return (llvm::Function *)LogErrorV(
            "function cannot be redefined with different # args");

    if (!f->empty())
        return (llvm::Function *)LogErrorV("function cannot be redefined");

    auto argIter = f->arg_begin();
    for (unsigned i = 0; i != proto->getArgs().size(); ++i, ++argIter)
        argIter->setName(proto->getArgs()[i]);

    llvm::BasicBlock *bb = llvm::BasicBlock::Create(*Context, "entry", f);
    Builder->SetInsertPoint(bb);

    NamedValues.clear();
    for (auto &arg : f->args())
        NamedValues[std::string(arg.getName())] = &arg;

    if (llvm::Value *retVal = body->codegen()) {
        Builder->CreateRet(retVal);
        llvm::verifyFunction(*f);
        fpm->run(*f, *fam);
        return f;
    }

    // error reading body
    f->eraseFromParent();
    return nullptr;
}
