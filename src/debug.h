#pragma once

#include <memory>

#include "llvm/IR/DIBuilder.h"

constexpr bool debug = true;

extern std::unique_ptr<llvm::DIBuilder> dbuilder;

class ExprAST;

struct DebugInfo {
    public:
        llvm::DICompileUnit *cu;
        llvm::DIType *dblTy;
        std::vector<llvm::DIScope *> lexicalBlocks;

        llvm::DIType *getDoubleTy();
        void emitLocation(ExprAST *ast);
};

extern DebugInfo ksDbgInfo;

struct SourceLocation {
    public:
        int line;
        int col;
};

extern SourceLocation curLoc;
extern SourceLocation lexLoc;

void debugSetup();
void debugFinalize();
