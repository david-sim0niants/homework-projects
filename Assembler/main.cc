#include <iostream>
#include <cstdlib>
#include <fstream>
#include <exception>


#include "assembler.h"


int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cerr << "Error: not enough arguments\n";
        return EXIT_FAILURE;
    }

    std::ifstream input_file {argv[1]};
    if (!input_file)
    {
        std::cerr << "Error: no file at location - " << argv[1] << '\n';
        return EXIT_FAILURE;
    }

    std::ofstream output_file {argv[2], std::ios::binary};
    if (!output_file)
    {
        std::cerr << "Error: no file at location - " << argv[2] << '\n';
        return EXIT_FAILURE;
    }

    std::vector<std::string> messages;
    assemble(input_file, output_file, messages);

    if (!messages.empty())
        std::cerr << "Assembler messages:\n";

    for (auto &&msg : messages)
    {
        std::cerr << msg << '\n';
    }

    return EXIT_SUCCESS;
}
