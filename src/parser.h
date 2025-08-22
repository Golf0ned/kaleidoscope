#pragma once

#include <memory>

#include "ast.h"
#include "lexer.h"

class Parser {
    public:
        Parser(Lexer &lexer);
        int getNextToken();
        void run();
        void parseStream();

    private:
        int getTokPrecedence();
        std::unique_ptr<ExprAST> logError(const char *str);
        std::unique_ptr<PrototypeAST> logErrorP(const char *str);
        std::unique_ptr<ExprAST> parseNumberExpr();
        std::unique_ptr<ExprAST> parseParenExpr();
        std::unique_ptr<ExprAST> parseIdentifierExpr();
        std::unique_ptr<ExprAST> parsePrimary();
        std::unique_ptr<ExprAST> parseExpression();
        std::unique_ptr<ExprAST>
        parseExpressionRest(int minPrecedence, std::unique_ptr<ExprAST> prev);
        std::unique_ptr<PrototypeAST> parsePrototype();
        std::unique_ptr<FunctionAST> parseDefinition();
        std::unique_ptr<PrototypeAST> parseExtern();
        std::unique_ptr<FunctionAST> parseTopLevelExpr();

        Lexer lexer;
        int curTok;
};
