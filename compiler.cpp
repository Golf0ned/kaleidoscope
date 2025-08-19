#include <cstdio>

#include "ast/ast.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

int main() {
    Lexer lexer;
    Parser parser(lexer);

    fprintf(stderr, "kaleidoscope> ");
    parser.getNextToken();

    initializeModule();

    parser.run();

    dumpIR();
    return 0;
}
