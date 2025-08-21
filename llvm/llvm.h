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

extern std::unique_ptr<llvm::FunctionPassManager> FPM;
extern std::unique_ptr<llvm::LoopAnalysisManager> LAM;
extern std::unique_ptr<llvm::FunctionAnalysisManager> FAM;
extern std::unique_ptr<llvm::CGSCCAnalysisManager> CGAM;
extern std::unique_ptr<llvm::ModuleAnalysisManager> MAM;
extern std::unique_ptr<llvm::PassInstrumentationCallbacks> PIC;
extern std::unique_ptr<llvm::StandardInstrumentations> SI;

void initializeModule();
void dumpIR();
void writeToBitcode(const char *filename);

