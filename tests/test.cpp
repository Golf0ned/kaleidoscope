#include <iostream>

extern "C" {
    double fib(double);
}

int main() {
    std::cout << fib(5) << std::endl;
}
