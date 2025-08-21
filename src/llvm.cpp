#include "llvm.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Support/raw_ostream.h"

std::unique_ptr<llvm::LLVMContext> Context;
std::unique_ptr<llvm::IRBuilder<>> Builder;
std::unique_ptr<llvm::Module> Module;
std::map<std::string, llvm::Value *> NamedValues;
std::unique_ptr<llvm::FunctionPassManager> fpm;
std::unique_ptr<llvm::LoopAnalysisManager> lam;
std::unique_ptr<llvm::FunctionAnalysisManager> fam;
std::unique_ptr<llvm::CGSCCAnalysisManager> cgam;
std::unique_ptr<llvm::ModuleAnalysisManager> mam;
std::unique_ptr<llvm::PassInstrumentationCallbacks> pic;
std::unique_ptr<llvm::StandardInstrumentations> si;

void initializeModule() {
    Context = std::make_unique<llvm::LLVMContext>();

    Module = std::make_unique<llvm::Module>("kaleidoscope", *Context);

    Builder = std::make_unique<llvm::IRBuilder<>>(*Context);

    fpm = std::make_unique<llvm::FunctionPassManager>();
    lam = std::make_unique<llvm::LoopAnalysisManager>();
    fam = std::make_unique<llvm::FunctionAnalysisManager>();
    cgam = std::make_unique<llvm::CGSCCAnalysisManager>();
    mam = std::make_unique<llvm::ModuleAnalysisManager>();
    pic = std::make_unique<llvm::PassInstrumentationCallbacks>();
    si = std::make_unique<llvm::StandardInstrumentations>(*Context, true);

    fpm->addPass(llvm::InstCombinePass());
    fpm->addPass(llvm::ReassociatePass());
    fpm->addPass(llvm::GVNPass());
    fpm->addPass(llvm::SimplifyCFGPass());

    llvm::PassBuilder pb;
    pb.registerModuleAnalyses(*mam);
    pb.registerFunctionAnalyses(*fam);
    pb.crossRegisterProxies(*lam, *fam, *cgam, *mam);
}

void dumpIR() {
    Module->print(llvm::errs(), nullptr);
}

void writeToBitcode(const char *filename) {
    std::error_code ec;
    llvm::raw_fd_ostream os(filename, ec);
    llvm::WriteBitcodeToFile(*Module.get(), os);
    os.close();
}

