#include "llvm.h"
#include "ast.h"
#include "kaleidoscopeJIT.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"
#include <cassert>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/CodeGen.h>
#include <memory>

std::unique_ptr<llvm::LLVMContext> Context;
std::unique_ptr<llvm::IRBuilder<>> Builder;
std::unique_ptr<llvm::Module> Module;
std::map<std::string, llvm::AllocaInst *> NamedValues;
std::unique_ptr<llvm::FunctionPassManager> fpm;
std::unique_ptr<llvm::LoopAnalysisManager> lam;
std::unique_ptr<llvm::FunctionAnalysisManager> fam;
std::unique_ptr<llvm::CGSCCAnalysisManager> cgam;
std::unique_ptr<llvm::ModuleAnalysisManager> mam;
std::unique_ptr<llvm::PassInstrumentationCallbacks> pic;
std::unique_ptr<llvm::StandardInstrumentations> si;
std::unique_ptr<llvm::PassBuilder> pb;

std::unique_ptr<llvm::orc::KaleidoscopeJIT> jit;
llvm::ExitOnError exitOnErr;
std::map<std::string, std::unique_ptr<PrototypeAST>> functionProtos;

void initializeModule() {
    Context = std::make_unique<llvm::LLVMContext>();

    Module = std::make_unique<llvm::Module>("kaleidoscope", *Context);
    if (jit) {
        Module->setDataLayout(jit->getDataLayout());
    }

    Builder = std::make_unique<llvm::IRBuilder<>>(*Context);

    fpm = std::make_unique<llvm::FunctionPassManager>();
    lam = std::make_unique<llvm::LoopAnalysisManager>();
    fam = std::make_unique<llvm::FunctionAnalysisManager>();
    cgam = std::make_unique<llvm::CGSCCAnalysisManager>();
    mam = std::make_unique<llvm::ModuleAnalysisManager>();
    pic = std::make_unique<llvm::PassInstrumentationCallbacks>();
    si = std::make_unique<llvm::StandardInstrumentations>(*Context, true);

    fpm->addPass(llvm::PromotePass());
    fpm->addPass(llvm::InstCombinePass());
    fpm->addPass(llvm::ReassociatePass());
    fpm->addPass(llvm::GVNPass());
    fpm->addPass(llvm::SimplifyCFGPass());

    pb.reset(new llvm::PassBuilder());
    pb->registerModuleAnalyses(*mam);
    pb->registerCGSCCAnalyses(*cgam);
    pb->registerFunctionAnalyses(*fam);
    pb->registerLoopAnalyses(*lam);
    pb->crossRegisterProxies(*lam, *fam, *cgam, *mam);
}

void initializeJIT() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    jit = exitOnErr(llvm::orc::KaleidoscopeJIT::Create());
    Module->setDataLayout(jit->getDataLayout());
}

llvm::Function *getFunction(std::string name) {
    if (auto *f = Module->getFunction(name))
        return f;

    auto fIter = functionProtos.find(name);
    if (fIter != functionProtos.end())
        return fIter->second->codegen();

    return nullptr;
}

llvm::AllocaInst *createEntryBlockAlloca(llvm::Function *function,
                                         llvm::StringRef varName) {
    llvm::IRBuilder<> builder(&function->getEntryBlock(),
                              function->getEntryBlock().begin());
    return builder.CreateAlloca(llvm::Type::getDoubleTy(*Context), nullptr,
                                varName);
}

void runModulePasses() {
    llvm::ModulePassManager mpm =
        pb->buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O2);
    mpm.run(*Module, *mam);
}

void dumpIR() { Module->print(llvm::errs(), nullptr); }

void writeToBitcode(const char *filename) {
    std::error_code ec;
    llvm::raw_fd_ostream os(filename, ec);
    llvm::WriteBitcodeToFile(*Module.get(), os);
    os.close();
}

void writeObject(const char *filename) {
    auto targetTriple = llvm::sys::getDefaultTargetTriple();

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
    if (!target) {
        llvm::errs() << error;
        abort();
    }

    auto cpu = "generic";
    auto features = "";

    llvm::TargetOptions opt;
    auto targetMachine = target->createTargetMachine(
        targetTriple, cpu, features, opt, llvm::Reloc::PIC_);

    Module->setDataLayout(targetMachine->createDataLayout());
    Module->setTargetTriple(targetTriple);

    std::error_code ec;
    llvm::raw_fd_ostream os(filename, ec);
    if (ec) {
        llvm::errs() << "Could not open file: " << ec.message();
        abort();
    }

    llvm::legacy::PassManager pass;
    auto fileType = llvm::CodeGenFileType::ObjectFile;
    if (targetMachine->addPassesToEmitFile(pass, os, nullptr, fileType)) {
        llvm::errs() << "TargetMachine can't emit a file of this type";
        abort();
    }

    pass.run(*Module);
    os.flush();
}
