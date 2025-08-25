#include <cstdio>

#include "runtime.h"

double print(double val) {
    fprintf(stdout, "%f", val);
    fflush(stdout);
    return 0.0;
}

double printStar() {
    fprintf(stdout, "*");
    fflush(stdout);
    return 0.0;
}

double printSpace() {
    fprintf(stdout, "*");
    fflush(stdout);
    return 0.0;
}

double printNewLine() {
    fprintf(stdout, "\n");
    fflush(stdout);
    return 0.0;
}
