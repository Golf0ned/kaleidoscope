#pragma once

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

extern std::unique_ptr<llvm::LLVMContext> Context;
extern std::unique_ptr<llvm::IRBuilder<>> Builder;
extern std::unique_ptr<llvm::Module> Module;
extern std::map<std::string, llvm::Value *> NamedValues;

extern std::unique_ptr<llvm::FunctionPassManager> fpm;
extern std::unique_ptr<llvm::LoopAnalysisManager> lam;
extern std::unique_ptr<llvm::FunctionAnalysisManager> fam;
extern std::unique_ptr<llvm::CGSCCAnalysisManager> cgam;
extern std::unique_ptr<llvm::ModuleAnalysisManager> mam;
extern std::unique_ptr<llvm::PassInstrumentationCallbacks> pic;
extern std::unique_ptr<llvm::StandardInstrumentations> si;

void initializeModule();
void dumpIR();
void writeToBitcode(const char *filename);
