#pragma once

#include <string>

enum Token {
    // Default: ASCII value

    // EOF
    tok_eof = -1,

    // Keyword
    tok_def = -2,
    tok_extern = -3,

    // Value
    tok_identifier = -4,
    tok_number = -5,

    // Control
    tok_if = -6,
    tok_then = -7,
    tok_else = -8,
    tok_for = -9,
    tok_in = -10,

    // operators
    tok_binary = -11,
    tok_unary = -12,
};

class Lexer {
    public:
        Lexer(FILE *inStream);
        int getTok();
        std::string getIdentifierValue();
        double getNumericValue();

    private:
        void readIdentifierOrKeyword();
        void readNumeric();
        void readComment();

        FILE *inStream;
        char lastChar;
        std::string identifierStr;
        double numVal;
};
