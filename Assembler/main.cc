#include <iostream>
#include <fstream>
#include <exception>


#include "assembler.h"


int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cerr << "Error: not enough arguments\n";
        std::terminate();
    }

    std::ifstream input_file {argv[1]};
    if (!input_file)
    {
        std::cerr << "Error: no file at location - " << argv[1] << '\n';
        std::terminate();
    }

    std::ofstream output_file {argv[2], std::ios::binary};
    if (!output_file)
    {
        std::cerr << "Error: no file at location - " << argv[2] << '\n';
        std::terminate();
    }

    assemble(input_file, output_file);
}
