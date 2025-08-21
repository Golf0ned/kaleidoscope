#include "llvm.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Support/raw_ostream.h"

std::unique_ptr<llvm::LLVMContext> Context;
std::unique_ptr<llvm::IRBuilder<>> Builder;
std::unique_ptr<llvm::Module> Module;
std::map<std::string, llvm::Value *> NamedValues;

void initializeModule() {
    Context = std::make_unique<llvm::LLVMContext>();
    Builder = std::make_unique<llvm::IRBuilder<>>(*Context);
    Module = std::make_unique<llvm::Module>("ben says hi", *Context);
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

