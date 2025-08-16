#include <cstdio>
#include <memory>
#include <unordered_map>

#include "../lexer/lexer.h"
#include "parser.h"


static std::unordered_map<char, int> binopPrecedence = {
    {'*', 40},
    {'+', 20},
    {'-', 20},
    {'<', 10},
};

Parser::Parser(Lexer& lexer)
    : lexer(lexer) {}

void Parser::run() {
    while (true) {
        fprintf(stderr, "kaleidoscope> ");
        switch (curTok) {
            case tok_eof:
                return;
            case ';':
                getNextToken();
                break;
            case tok_def:
                if (parseDefinition())
                    fprintf(stderr, "Parsed function\n");
                else
                    getNextToken();
                break;
            case tok_extern:
                if (parseExtern())
                    fprintf(stderr, "Parsed an extern\n");
                else
                    getNextToken();
                break;
            default:
                if (parseTopLevelExpr())
                    fprintf(stderr, "Parsed top level expr\n");
                else
                    getNextToken();
                break;
        }
    }
}

int Parser::getNextToken() {
    return curTok = lexer.getTok();
}

int Parser::getTokPrecedence() {
    if (!isascii(curTok))
        return -1;

    auto pair = binopPrecedence.find(curTok);
    if (pair == binopPrecedence.end())
        return -1;
    return pair->second;
}

std::unique_ptr<ExprAST> Parser::logError(const char *str) {
    fprintf(stderr, "Error: %s\n", str);
    return nullptr;
}

std::unique_ptr<PrototypeAST> Parser::logErrorP(const char *str) {
    logError(str);
    return nullptr;
}

std::unique_ptr<ExprAST> Parser::parseNumberExpr() {
    auto res = std::make_unique<NumberExprAST>(lexer.getNumericValue());
    getNextToken();
    return std::move(res);
}

std::unique_ptr<ExprAST> Parser::parseParenExpr() {
    getNextToken();
    auto v = parseExpression();
    if (!v)
        return nullptr;
    
    if (curTok != ')')
        return logError("expected ')'");
    getNextToken();
    return v; 
}

std::unique_ptr<ExprAST> Parser::parseIdentifierExpr() {
    std::string name = lexer.getIdentifierValue();

    getNextToken();
    if (curTok != '(') // variable
        return std::make_unique<VariableExprAST>(name);

    // else, function call
    getNextToken();
    std::vector<std::unique_ptr<ExprAST>> args;
    while (curTok != ')') {
        if (auto arg = parseExpression())
            args.push_back(std::move(arg));
        else
            return nullptr;

        if (curTok == ')')
            break;

        if (curTok != ',')
            return logError("expected ')' or ',' in argument list");

        getNextToken();
    }

    getNextToken();
    return std::make_unique<CallExprAST>(name, std::move(args));
}

std::unique_ptr<ExprAST> Parser::parsePrimary() {
    switch(curTok) {
        default:
            return logError("unknown token when expecting an expression");
        case tok_identifier:
            return parseIdentifierExpr();
        case tok_number:
            return parseNumberExpr();
        case '(':
            return parseParenExpr();
    }
}

std::unique_ptr<ExprAST> Parser::parseExpression() {
    auto first = parsePrimary();
    if (!first)
        return nullptr;

    return parseExpressionRest(0, std::move(first));
}

std::unique_ptr<ExprAST> Parser::parseExpressionRest(int minPrecedence,
                                                     std::unique_ptr<ExprAST> prev) {
    while (true) {
        int curPrecedence = getTokPrecedence();
        if (curPrecedence < minPrecedence)
            return prev;

        int binOp = curTok;
        getNextToken();

        auto next = parsePrimary();
        if (!next)
            return nullptr;

        int nextPrecedence = getTokPrecedence();
        if (curPrecedence < nextPrecedence) {
            next = parseExpressionRest(curPrecedence + 1,
                                       std::move(next));
            if (!next)
                return nullptr;
        }
        prev = std::make_unique<BinaryExprAST>(binOp, std::move(prev),
                                                      std::move(next));
    }
}

std::unique_ptr<PrototypeAST> Parser::parsePrototype() {
    if (curTok != tok_identifier)
        return logErrorP("Expected function name in prototype");

    std::string name = lexer.getIdentifierValue();
    getNextToken();

    if (curTok != '(')
        return logErrorP("Expected '(' in prototype");

    std::vector<std::string> argNames;
    while (getNextToken() == tok_identifier)
        argNames.push_back(lexer.getIdentifierValue());
    
    if (curTok != ')')
        return logErrorP("Expected ')' in prototype");
    getNextToken();

    return std::make_unique<PrototypeAST>(name, std::move(argNames));
}

std::unique_ptr<FunctionAST> Parser::parseDefinition() {
    getNextToken();
    auto prototype = parsePrototype();
    if (!prototype)
        return nullptr;

    if (auto expression = parseExpression())
        return std::make_unique<FunctionAST>(std::move(prototype), std::move(expression));
    return nullptr;
}

std::unique_ptr<PrototypeAST> Parser::parseExtern() {
    getNextToken();
    return parsePrototype();
}

std::unique_ptr<FunctionAST> Parser::parseTopLevelExpr() {
    if (auto expression = parseExpression()) {
        auto prototype = std::make_unique<PrototypeAST>("", std::vector<std::string>());
        return std::make_unique<FunctionAST>(std::move(prototype), std::move(expression));
    }
    return nullptr;
}
