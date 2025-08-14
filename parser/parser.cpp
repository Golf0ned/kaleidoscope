#include <cstdio>
#include <memory>

#include "../lexer/lexer.h"
#include "parser.h"


Parser::Parser(Lexer& lexer)
    : lexer(lexer) {}

int Parser::getNextToken() {
    return curTok = lexer.getTok();
}

std::unique_ptr<ExprAST> Parser::logError(const char *str) {
    fprintf(stderr, "Error: %s\n", str);
    return nullptr;
}

std::unique_ptr<PrototypeAST> Parser::logErrorP(const char *str) {
    logError(str);
    return nullptr;
}
