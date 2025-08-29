#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>

#include "ast.h"
#include "llvm.h"

std::unordered_map<char, int> binopPrecedence = {
    {'*', 40}, {'+', 20}, {'-', 20}, {'<', 10}, {'=', 2},
};

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
    llvm::AllocaInst *a = NamedValues[name];
    if (!a)
        LogErrorV("Unknown variable name");
    return Builder->CreateLoad(a->getAllocatedType(), a, name.c_str());
}

const std::string VariableExprAST::getName() { return name; }

BinaryExprAST::BinaryExprAST(char op, std::unique_ptr<ExprAST> left,
                             std::unique_ptr<ExprAST> right)
    : op(op), left(std::move(left)), right(std::move(right)) {}

llvm::Value *BinaryExprAST::codegen() {
    if (op == '=') {
        VariableExprAST *leftExpr = static_cast<VariableExprAST *>(left.get());
        if (!leftExpr)
            return LogErrorV("destination of '=' must be variable");

        llvm::Value *val = right->codegen();
        if (!val)
            return nullptr;

        llvm::Value *var = NamedValues[leftExpr->getName()];
        if (!var)
            return LogErrorV("unknown variable name");

        Builder->CreateStore(val, var);
        return val;
    }

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
            break;
    }

    llvm::Function *f = getFunction(std::string("binary") + op);
    assert(f && "binary operator not found!");

    llvm::Value *ops[2] = {l, r};
    return Builder->CreateCall(f, ops, "binop");
}

UnaryExprAST::UnaryExprAST(char op, std::unique_ptr<ExprAST> operand)
    : op(op), operand(std::move(operand)) {}

llvm::Value *UnaryExprAST::codegen() {
    llvm::Value *operandV = operand->codegen();
    if (!operandV)
        return nullptr;

    llvm::Function *f = getFunction(std::string("unary") + op);
    if (!f)
        return LogErrorV("unknown unary operator");

    return Builder->CreateCall(f, operandV, "unop");
}

CallExprAST::CallExprAST(const std::string &callee,
                         std::vector<std::unique_ptr<ExprAST>> args)
    : callee(callee), args(std::move(args)) {}

llvm::Value *CallExprAST::codegen() {
    llvm::Function *calleeF = getFunction(callee);
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

IfExprAST::IfExprAST(std::unique_ptr<ExprAST> cond,
                     std::unique_ptr<ExprAST> tBranch,
                     std::unique_ptr<ExprAST> fBranch)
    : cond(std::move(cond)), tBranch(std::move(tBranch)),
      fBranch(std::move(fBranch)) {}

llvm::Value *IfExprAST::codegen() {
    llvm::Value *c = cond->codegen();
    if (!c)
        return nullptr;

    c = Builder->CreateFCmpONE(
        c, llvm::ConstantFP::get(*Context, llvm::APFloat(0.0)), "ifcond");

    llvm::Function *function = Builder->GetInsertBlock()->getParent();
    llvm::BasicBlock *tBB =
        llvm::BasicBlock::Create(*Context, "then", function);
    llvm::BasicBlock *fBB = llvm::BasicBlock::Create(*Context, "else");
    llvm::BasicBlock *contBB = llvm::BasicBlock::Create(*Context, "ifcont");

    Builder->CreateCondBr(c, tBB, fBB);

    Builder->SetInsertPoint(tBB);
    llvm::Value *t = tBranch->codegen();
    if (!t)
        return nullptr;
    Builder->CreateBr(contBB);
    tBB = Builder->GetInsertBlock();

    function->insert(function->end(), fBB);
    Builder->SetInsertPoint(fBB);
    llvm::Value *f = fBranch->codegen();
    if (!f)
        return nullptr;
    Builder->CreateBr(contBB);
    fBB = Builder->GetInsertBlock();

    function->insert(function->end(), contBB);
    Builder->SetInsertPoint(contBB);
    llvm::PHINode *p =
        Builder->CreatePHI(llvm::Type::getDoubleTy(*Context), 2, "iftmp");

    p->addIncoming(t, tBB);
    p->addIncoming(f, fBB);
    return p;
}

ForExprAST::ForExprAST(std::string &varName, std::unique_ptr<ExprAST> start,
                       std::unique_ptr<ExprAST> end,
                       std::unique_ptr<ExprAST> step,
                       std::unique_ptr<ExprAST> body)
    : varName(varName), start(std::move(start)), end(std::move(end)),
      step(std::move(step)), body(std::move(body)) {}

llvm::Value *ForExprAST::codegen() {
    llvm::Function *function = Builder->GetInsertBlock()->getParent();

    llvm::AllocaInst *alloca = createEntryBlockAlloca(function, varName);
    llvm::Value *startVal = start->codegen();
    if (!startVal)
        return nullptr;
    Builder->CreateStore(startVal, alloca);

    llvm::BasicBlock *prevBB = Builder->GetInsertBlock();
    llvm::BasicBlock *loopBB =
        llvm::BasicBlock::Create(*Context, "loop", function);

    Builder->CreateBr(loopBB);

    Builder->SetInsertPoint(loopBB);

    llvm::AllocaInst *oldVal = NamedValues[varName];
    NamedValues[varName] = alloca;

    if (!body->codegen())
        return nullptr;

    llvm::Value *stepVal = nullptr;
    if (step) {
        stepVal = step->codegen();
        if (!stepVal)
            return nullptr;
    } else {
        stepVal = llvm::ConstantFP::get(*Context, llvm::APFloat(1.0));
    }

    llvm::Value *curVar = Builder->CreateLoad(alloca->getAllocatedType(),
                                              alloca, varName.c_str());
    llvm::Value *nextVar = Builder->CreateFAdd(curVar, stepVal, "nextvar");
    Builder->CreateStore(nextVar, alloca);

    llvm::Value *endCond = end->codegen();
    if (!endCond)
        return nullptr;

    endCond = Builder->CreateFCmpONE(
        endCond, llvm::ConstantFP::get(*Context, llvm::APFloat(0.0)),
        "loopcond");

    llvm::BasicBlock *endBB = Builder->GetInsertBlock();
    llvm::BasicBlock *afterBB =
        llvm::BasicBlock::Create(*Context, "afterloop", function);
    Builder->CreateCondBr(endCond, loopBB, afterBB);
    Builder->SetInsertPoint(afterBB);

    if (oldVal)
        NamedValues[varName] = oldVal;
    else
        NamedValues.erase(varName);

    return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(*Context));
}

VarExprAST::VarExprAST(std::vector<VarNamePair> varNames,
                       std::unique_ptr<ExprAST> body)
    : varNames(std::move(varNames)), body(std::move(body)) {}

llvm::Value *VarExprAST::codegen() {
    std::vector<llvm::AllocaInst *> oldBindings;
    llvm::Function *function = Builder->GetInsertBlock()->getParent();

    for (unsigned i = 0, e = varNames.size(); i != e; i++) {
        const std::string &name = varNames[i].first;
        ExprAST *init = varNames[i].second.get();

        llvm::Value *initVal =
            init ? init->codegen()
                 : llvm::ConstantFP::get(*Context, llvm::APFloat(0.0));
        if (!initVal)
            return nullptr;

        llvm::AllocaInst *alloca = createEntryBlockAlloca(function, name);
        Builder->CreateStore(initVal, alloca);

        oldBindings.push_back(NamedValues[name]);
        NamedValues[name] = alloca;
    }

    llvm::Value *bodyVal = body->codegen();
    if (!bodyVal)
        return nullptr;

    for (unsigned i = 0, e = varNames.size(); i != e; i++) {
        NamedValues[varNames[i].first] = oldBindings[i];
    }

    return bodyVal;
}

PrototypeAST::PrototypeAST(const std::string &name,
                           std::vector<std::string> args, bool isOperator,
                           unsigned precedence)
    : name(name), args(args), isOperator(isOperator), precedence(precedence) {}

const std::string &PrototypeAST::getName() const { return name; }

const std::vector<std::string> &PrototypeAST::getArgs() { return args; }

bool PrototypeAST::isUnaryOp() const { return isOperator && args.size() == 1; }

bool PrototypeAST::isBinaryOp() const { return isOperator && args.size() == 2; }

char PrototypeAST::getOperatorName() const {
    assert(isUnaryOp() || isBinaryOp());
    return name[name.size() - 1];
}

unsigned PrototypeAST::getBinaryPrecedence() const { return precedence; }

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
    auto &p = *proto;
    functionProtos[proto->getName()] = std::move(proto);
    llvm::Function *f = getFunction(p.getName());
    if (!f)
        return nullptr;

    auto newArgs = p.getArgs();
    if (f->arg_size() != newArgs.size())
        return (llvm::Function *)LogErrorV(
            "function cannot be redefined with different # args");

    if (!f->empty())
        return (llvm::Function *)LogErrorV("function cannot be redefined");

    auto argIter = f->arg_begin();
    for (unsigned i = 0; i != newArgs.size(); ++i, ++argIter)
        argIter->setName(newArgs[i]);

    if (p.isBinaryOp())
        binopPrecedence[p.getOperatorName()] = p.getBinaryPrecedence();

    llvm::BasicBlock *bb = llvm::BasicBlock::Create(*Context, "entry", f);
    Builder->SetInsertPoint(bb);

    NamedValues.clear();
    for (auto &arg : f->args()) {
        llvm::AllocaInst *alloca = createEntryBlockAlloca(f, arg.getName());
        Builder->CreateStore(&arg, alloca);
        NamedValues[std::string(arg.getName())] = alloca;
    }

    if (llvm::Value *retVal = body->codegen()) {
        Builder->CreateRet(retVal);
        llvm::verifyFunction(*f);
        if (jit)
            fpm->run(*f, *fam);
        return f;
    }

    // error reading body
    f->eraseFromParent();
    return nullptr;
}
