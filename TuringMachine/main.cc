#include "turing_machine.h"
#include <iostream>
#include <fstream>


int main(int argc, const char *argv[])
{
    std::ifstream file;
    if (argc > 1)
    {
        file = std::ifstream(argv[1]);
        if (!file)
            std::cerr << "Error couldn't open file " << argv[1] << ". Using stdin.\n";
    }

    std::istream &is = argc > 1 ? file : std::cin;
    TuringMachine turing_machine(is);

    turing_machine.print_Info(std::cout);
}