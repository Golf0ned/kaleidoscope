#include <cstdio>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <memory>
#include <unordered_map>

#include "ast.h"
#include "lexer.h"
#include "llvm.h"
#include "parser.h"

Parser::Parser(Lexer &lexer) : lexer(lexer) {}

void Parser::run() {
    while (true) {
        switch (curTok) {
            case tok_eof:
                return;
            case ';':
                getNextToken();
                break;
            case tok_def:
                if (auto ast = parseDefinition()) {
                    if (auto *ir = ast->codegen()) {
                        exitOnErr(jit->addModule(llvm::orc::ThreadSafeModule(
                            std::move(Module), std::move(Context))));
                        initializeModule();
                    }
                } else
                    getNextToken();
                fprintf(stderr, "kaleidoscope> ");
                break;
            case tok_extern:
                if (auto ast = parseExtern()) {
                    if (auto *ir = ast->codegen()) {
                        functionProtos[ast->getName()] = std::move(ast);
                    }
                } else
                    getNextToken();
                fprintf(stderr, "kaleidoscope> ");
                break;
            default:
                if (auto ast = parseTopLevelExpr()) {
                    if (auto *ir = ast->codegen()) {
                        auto rt =
                            jit->getMainJITDylib().createResourceTracker();
                        auto tsm = llvm::orc::ThreadSafeModule(
                            std::move(Module), std::move(Context));
                        exitOnErr(jit->addModule(std::move(tsm), rt));
                        initializeModule();

                        auto exprSymbol = exitOnErr(jit->lookup("__anon_expr"));

                        double (*fp)() =
                            exprSymbol.getAddress().toPtr<double (*)()>();
                        fprintf(stderr, "Evaluated to %f\n", fp());

                        exitOnErr(rt->remove());
                    }
                } else
                    getNextToken();
                fprintf(stderr, "kaleidoscope> ");
                break;
        }
    }
}

void Parser::parseStream() {
    while (true) {
        switch (curTok) {
            case tok_eof:
                return;
            case ';':
                getNextToken();
                break;
            case tok_def:
                if (auto ast = parseDefinition()) {
                    ast->codegen();
                } else
                    getNextToken();
                break;
            case tok_extern:
                if (auto ast = parseExtern()) {
                    ast->codegen();
                } else
                    getNextToken();
                break;
            default:
                if (auto ast = parseTopLevelExpr()) {
                    ast->codegen();
                } else
                    getNextToken();
                break;
        }
    }
}

int Parser::getNextToken() { return curTok = lexer.getTok(); }

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

std::unique_ptr<ExprAST> Parser::parseIfExpr() {
    getNextToken();

    auto cond = parseExpression();
    if (!cond)
        return nullptr;

    if (curTok != tok_then)
        return logError("expected then");
    getNextToken();

    auto tBranch = parseExpression();
    if (!tBranch)
        return nullptr;

    if (curTok != tok_else)
        return logError("expected else");
    getNextToken();

    auto fBranch = parseExpression();
    if (!fBranch)
        return nullptr;

    return std::make_unique<IfExprAST>(std::move(cond), std::move(tBranch),
                                       std::move(fBranch));
}

std::unique_ptr<ExprAST> Parser::parseForExpr() {
    getNextToken();

    if (curTok != tok_identifier)
        return logError("expected start variable name");
    std::string name = lexer.getIdentifierValue();
    getNextToken();

    if (curTok != '=')
        return logError("expected = after start name");
    getNextToken();

    auto start = parseExpression();
    if (!start)
        return nullptr;

    if (curTok != ',')
        return logError("expected , after start variable");
    getNextToken();

    auto end = parseExpression();
    if (!end)
        return nullptr;

    std::unique_ptr<ExprAST> step;
    if (curTok == ',') {
        getNextToken();
        step = parseExpression();
        if (!step)
            return nullptr;
    }

    if (curTok != tok_in)
        return logError("expected in after for");

    getNextToken();

    auto body = parseExpression();
    if (!body)
        return nullptr;

    return std::make_unique<ForExprAST>(name, std::move(start), std::move(end),
                                        std::move(step), std::move(body));
}

std::unique_ptr<ExprAST> Parser::parseVarExpr() {
    std::vector<VarNamePair> varNames;

    do {
        getNextToken(); // eats "var" or comma
        if (curTok != tok_identifier)
            return logError("expected identifier after var");

        std::string name = lexer.getIdentifierValue();
        getNextToken();

        std::unique_ptr<ExprAST> init;
        if (curTok == '=') {
            getNextToken();

            init = parseExpression();
            if (!init)
                return nullptr;
        }

        varNames.emplace_back(name, std::move(init));
    } while (curTok == ',');

    if (curTok != tok_in)
        return logError("expected 'in' after 'var'");
    getNextToken();

    auto body = parseExpression();
    if (!body)
        return nullptr;

    return std::make_unique<VarExprAST>(std::move(varNames), std::move(body));
}

std::unique_ptr<ExprAST> Parser::parseUnary() {
    if (!isascii(curTok) || curTok == '(' || curTok == ',')
        return parsePrimary();

    int opChar = curTok;
    getNextToken();
    if (auto operand = parseUnary())
        return std::make_unique<UnaryExprAST>(opChar, std::move(operand));
    return nullptr;
}

std::unique_ptr<ExprAST> Parser::parsePrimary() {
    switch (curTok) {
        default:
            return logError("unknown token when expecting an expression");
        case tok_identifier:
            return parseIdentifierExpr();
        case tok_number:
            return parseNumberExpr();
        case '(':
            return parseParenExpr();
        case tok_if:
            return parseIfExpr();
        case tok_for:
            return parseForExpr();
        case tok_var:
            return parseVarExpr();
    }
}

std::unique_ptr<ExprAST> Parser::parseExpression() {
    auto first = parseUnary();
    if (!first)
        return nullptr;

    return parseExpressionRest(0, std::move(first));
}

std::unique_ptr<ExprAST>
Parser::parseExpressionRest(int minPrecedence, std::unique_ptr<ExprAST> prev) {
    while (true) {
        int curPrecedence = getTokPrecedence();
        if (curPrecedence < minPrecedence)
            return prev;

        int binOp = curTok;
        getNextToken();

        auto next = parseUnary();
        if (!next)
            return nullptr;

        int nextPrecedence = getTokPrecedence();
        if (curPrecedence < nextPrecedence) {
            next = parseExpressionRest(curPrecedence + 1, std::move(next));
            if (!next)
                return nullptr;
        }
        prev = std::make_unique<BinaryExprAST>(binOp, std::move(prev),
                                               std::move(next));
    }
}

std::unique_ptr<PrototypeAST> Parser::parsePrototype() {
    std::string name;
    unsigned kind = 0, binaryPrecedence = 30;

    switch (curTok) {
        default:
            return logErrorP("Expected function name in prototype");
        case tok_identifier:
            name = lexer.getIdentifierValue();
            kind = 0;
            getNextToken();
            break;
        case tok_unary:
            getNextToken();
            if (!isascii(curTok))
                return logErrorP("Expected unary operator");
            name = "unary";
            name += (char)curTok;
            kind = 1;
            getNextToken();
            break;
        case tok_binary:
            getNextToken();
            if (!isascii(curTok))
                return logErrorP("Expected binary operator");
            name = "binary";
            name += (char)curTok;
            kind = 2;
            getNextToken();
            if (curTok == tok_number) {
                int val = lexer.getNumericValue();
                if (val < 1 || val > 100)
                    return logErrorP("precedence out of bounds (1...100)");
                binaryPrecedence = (unsigned)val;
                getNextToken();
            }
            break;
    }

    if (curTok != '(')
        return logErrorP("Expected '(' in prototype");

    std::vector<std::string> argNames;
    while (getNextToken() == tok_identifier)
        argNames.push_back(lexer.getIdentifierValue());

    if (curTok != ')')
        return logErrorP("Expected ')' in prototype");
    getNextToken();

    if (kind && argNames.size() != kind)
        return logErrorP("Invalid number of operands for operator");

    return std::make_unique<PrototypeAST>(name, std::move(argNames), kind != 0,
                                          binaryPrecedence);
}

std::unique_ptr<FunctionAST> Parser::parseDefinition() {
    getNextToken();
    auto prototype = parsePrototype();
    if (!prototype)
        return nullptr;

    if (auto expression = parseExpression())
        return std::make_unique<FunctionAST>(std::move(prototype),
                                             std::move(expression));
    return nullptr;
}

std::unique_ptr<PrototypeAST> Parser::parseExtern() {
    getNextToken();
    return parsePrototype();
}

std::unique_ptr<FunctionAST> Parser::parseTopLevelExpr() {
    if (auto expression = parseExpression()) {
        std::string name = jit ? "__anon_expr" : "main";
        auto prototype =
            std::make_unique<PrototypeAST>(name, std::vector<std::string>());
        return std::make_unique<FunctionAST>(std::move(prototype),
                                             std::move(expression));
    }
    return nullptr;
}
