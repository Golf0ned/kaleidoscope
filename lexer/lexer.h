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
};

class Lexer {
    public:
        int getTok();
        std::string getIdentifierValue();
        double getNumericValue();

    private:
        void readIdentifierOrKeyword();
        void readNumeric();
        void readComment();

        char lastChar;
        std::string identifierStr;
        double numVal;
};
