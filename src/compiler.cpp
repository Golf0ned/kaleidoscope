#include <cstdio>

#include "debug.h"
#include "lexer.h"
#include "llvm.h"
#include "parser.h"

const std::string bitcodeOutFileName = "kaleidoscope.bc";
const std::string objectOutFileName = "kaleidoscope.o";

void runInteractive() {
    Lexer lexer(stdin);
    Parser parser(lexer);

    fprintf(stderr, "kaleidoscope> ");
    parser.getNextToken();

    initializeModule();
    initializeJIT();
    parser.run();
    fprintf(stderr, "\n");
}

void runFileInput(char *inFileName) {
    initializeModule();

    auto *inFile = fopen(inFileName, "r");
    if (!inFile)
        fprintf(stderr, "Error: file open failed");

    Lexer lexer(inFile);
    Parser parser(lexer);

    if (debug)
        debugSetup();

    parser.getNextToken();

    parser.parseStream();

    fclose(inFile);

    if (!debug)
        runModulePasses();

    writeToBitcode(bitcodeOutFileName.c_str());

    if (debug) {
        debugFinalize();
    }

    writeObject(objectOutFileName.c_str());
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
