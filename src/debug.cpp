#include <memory>

#include "llvm/IR/DIBuilder.h"

#include "debug.h"
#include "llvm.h"

std::unique_ptr<llvm::DIBuilder> dbuilder;

struct DebugInfo ksDbgInfo;

llvm::DIType *DebugInfo::getDoubleTy() {
    if (dblTy)
        return dblTy;

    dblTy = dbuilder->createBasicType("double", 64, llvm::dwarf::DW_ATE_float);
    return dblTy;
}

void debugSetup() {
    dbuilder = std::make_unique<llvm::DIBuilder>(*Module);
    ksDbgInfo.cu = dbuilder->createCompileUnit(
        llvm::dwarf::DW_LANG_C, dbuilder->createFile("kaleidoscope.ks", "."),
        "kaleidoscope", false, "", 0);
}

void debugFinalize() { dbuilder->finalize(); }
