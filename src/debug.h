#pragma once

#include <memory>

#include "llvm/IR/DIBuilder.h"

extern std::unique_ptr<llvm::DIBuilder> dbuilder;

extern struct DebugInfo {
        llvm::DICompileUnit *cu;
        llvm::DIType *dblTy;

        llvm::DIType *getDoubleTy();
} ksDbgInfo;

void debugSetup();
void debugFinalize();
