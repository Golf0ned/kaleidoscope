#include <cstdio>

#include "lexer/lexer.h"
#include "parser/parser.h"

int main() {
    Lexer lexer;
    Parser parser(lexer);

    fprintf(stderr, "kaleidoscope> ");
    parser.getNextToken();

    parser.run();
}
