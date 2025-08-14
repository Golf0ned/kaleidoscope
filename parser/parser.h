#pragma once

#include <memory>

#include "../ast/ast.h"
#include "../lexer/lexer.h"

// TODO: start from 2.4

class Parser {
    public:
        Parser(Lexer& lexer);
        int getNextToken();
        std::unique_ptr<ExprAST> logError(const char *str);
        std::unique_ptr<PrototypeAST> logErrorP(const char *str);
    private:
        Lexer lexer;
        int curTok;
};
