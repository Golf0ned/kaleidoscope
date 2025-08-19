#include <cstdio>

#include "ast/ast.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

const std::string filename = "kaleidoscope.bc";

int main() {
    Lexer lexer;
    Parser parser(lexer);

    fprintf(stderr, "kaleidoscope> ");
    parser.getNextToken();

    initializeModule();

    parser.run();

    // dumpIR();
    writeToBitcode(filename.c_str());
    return 0;
}
