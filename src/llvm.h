#pragma once

#include <llvm/ADT/StringRef.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"

#include "kaleidoscopeJIT.h"

#include "ast.h"

extern std::unique_ptr<llvm::LLVMContext> Context;
extern std::unique_ptr<llvm::IRBuilder<>> Builder;
extern std::unique_ptr<llvm::Module> Module;
extern std::map<std::string, llvm::AllocaInst *> NamedValues;

extern std::unique_ptr<llvm::FunctionPassManager> fpm;
extern std::unique_ptr<llvm::LoopAnalysisManager> lam;
extern std::unique_ptr<llvm::FunctionAnalysisManager> fam;
extern std::unique_ptr<llvm::CGSCCAnalysisManager> cgam;
extern std::unique_ptr<llvm::ModuleAnalysisManager> mam;
extern std::unique_ptr<llvm::PassInstrumentationCallbacks> pic;
extern std::unique_ptr<llvm::StandardInstrumentations> si;

extern std::unique_ptr<llvm::orc::KaleidoscopeJIT> jit;
extern llvm::ExitOnError exitOnErr;
extern std::map<std::string, std::unique_ptr<PrototypeAST>> functionProtos;

extern std::unique_ptr<llvm::DIBuilder> dbuilder;

extern struct DebugInfo {
        llvm::DICompileUnit *cu;
        llvm::DIType *dblTy;

        llvm::DIType *getDoubleTy();
} ksDbgInfo;

void initializeModule();
void initializeJIT();
void debugSetup();

llvm::Function *getFunction(std::string name);
llvm::AllocaInst *createEntryBlockAlloca(llvm::Function *function,
                                         llvm::StringRef varName);

llvm::DISubroutineType *createFunctionType(unsigned numArgs);

void runModulePasses();

void debugFinalize();

void dumpIR();
void writeToBitcode(const char *filename);
void writeObject(const char *filename);
