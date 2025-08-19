#include <cstdio>

#include "ast/ast.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

const std::string outFileName = "kaleidoscope.bc";

void runInteractive() {
    Lexer lexer(stdin);
    Parser parser(lexer);

    fprintf(stderr, "kaleidoscope> ");
    parser.getNextToken();

    initializeModule();

    parser.run();

    dumpIR();
}

void runFileInput(char *inFileName) {

    initializeModule();

    // TODO: open file
    // auto inFile = ...

    Lexer lexer(/* file */);
    Parser parser(lexer);

    // TODO: parse file stream
    // parser.parseStream();

    writeToBitcode(outFileName.c_str());
}

int main(int argc, char **argv) {
    switch (argc) {
        case 0:
        case 1:
            runInteractive();
            break;
        case 2:
            runFileInput(argv[1]);
            break;
        default:
            fprintf(stderr, "Error: too many args (max 1)");
            return 1;
    }
    return 0;
}
