#include <cstdio>

#include "runtime.h"

double print(double val) {
    fprintf(stdout, "%f", val);
    fflush(stdout);
    return 0.0;
}

double println(double val) {
    fprintf(stdout, "%f\n", val);
    fflush(stdout);
    return 0.0;
}

double put(double val) {
    char c = (char)val;
    fprintf(stdout, "%c", c);
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
