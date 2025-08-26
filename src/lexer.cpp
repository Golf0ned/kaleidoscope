#include <cctype>
#include <cstdio>
#include <cstdlib>

#include "lexer.h"

Lexer::Lexer(FILE *inStream) : inStream(inStream) { lastChar = ' '; }

std::string Lexer::getIdentifierValue() { return identifierStr; }

double Lexer::getNumericValue() { return numVal; }

void Lexer::readIdentifierOrKeyword() {
    identifierStr = lastChar;
    while (isalnum((lastChar = getc(inStream)))) {
        identifierStr += lastChar;
    }
}

void Lexer::readNumeric() {
    std::string buffer;

    do {
        buffer += lastChar;
        lastChar = getc(inStream);
    } while (isdigit(lastChar) || lastChar == '.');

    numVal = std::strtod(buffer.c_str(), nullptr);
}

void Lexer::readComment() {
    do {
        lastChar = getc(inStream);
    } while (lastChar != EOF && lastChar != '\n' && lastChar != '\r');
}

int Lexer::getTok() {
    while (isspace(lastChar)) {
        lastChar = getc(inStream);
    }

    if (isalpha(lastChar)) {
        readIdentifierOrKeyword();
        if (identifierStr == "def")
            return tok_def;
        if (identifierStr == "extern")
            return tok_extern;
        if (identifierStr == "if")
            return tok_if;
        if (identifierStr == "then")
            return tok_then;
        if (identifierStr == "else")
            return tok_else;
        if (identifierStr == "for")
            return tok_for;
        if (identifierStr == "in")
            return tok_in;
        if (identifierStr == "binary")
            return tok_binary;
        if (identifierStr == "unary")
            return tok_unary;
        return tok_identifier;
    }

    if (isdigit(lastChar) || lastChar == '.') {
        readNumeric();
        return tok_number;
    }

    if (lastChar == '#') {
        readComment();
        if (lastChar != EOF)
            return getTok();
    }

    if (lastChar == EOF) {
        return tok_eof;
    }

    int curChar = lastChar;
    lastChar = getc(inStream);
    return curChar;
}
