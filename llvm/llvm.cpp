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
std::unique_ptr<llvm::FunctionPassManager> FPM;
std::unique_ptr<llvm::LoopAnalysisManager> LAM;
std::unique_ptr<llvm::FunctionAnalysisManager> FAM;
std::unique_ptr<llvm::CGSCCAnalysisManager> CGAM;
std::unique_ptr<llvm::ModuleAnalysisManager> MAM;
std::unique_ptr<llvm::PassInstrumentationCallbacks> PIC;
std::unique_ptr<llvm::StandardInstrumentations> SI;

void initializeModule() {
    Context = std::make_unique<llvm::LLVMContext>();

    Module = std::make_unique<llvm::Module>("kaleidoscope", *Context);

    Builder = std::make_unique<llvm::IRBuilder<>>(*Context);

    FPM = std::make_unique<llvm::FunctionPassManager>();
    LAM = std::make_unique<llvm::LoopAnalysisManager>();
    FAM = std::make_unique<llvm::FunctionAnalysisManager>();
    CGAM = std::make_unique<llvm::CGSCCAnalysisManager>();
    MAM = std::make_unique<llvm::ModuleAnalysisManager>();
    PIC = std::make_unique<llvm::PassInstrumentationCallbacks>();
    SI = std::make_unique<llvm::StandardInstrumentations>(*Context, true);

    FPM->addPass(llvm::InstCombinePass());
    FPM->addPass(llvm::ReassociatePass());
    FPM->addPass(llvm::GVNPass());
    FPM->addPass(llvm::SimplifyCFGPass());

    llvm::PassBuilder pb;
    pb.registerModuleAnalyses(*MAM);
    pb.registerFunctionAnalyses(*FAM);
    pb.crossRegisterProxies(*LAM, *FAM, *CGAM, *MAM);
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

