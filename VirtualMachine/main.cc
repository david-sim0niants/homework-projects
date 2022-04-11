#include <iostream>
#include <fstream>
#include <exception>


#include "virtual_machine.h"


int main(int argc, const char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Error: no arguments specified.\n";
        std::terminate();
    }


    const char *program_filename = argv[1];

    std::ifstream program_file {program_filename, std::ios::binary};

    if (!program_file)
    {
        std::cerr << "Error: no file at location " << program_filename << '\n';
        std::terminate();
    }


    program_file.unsetf(std::ios::skipws);
    program_file.seekg(0, std::ios::end);
    std::streampos stream_size = program_file.tellg();
    program_file.seekg(0, std::ios::beg);


    VirtualMachine vm (1 << 20, &std::cin, &std::cout);
    vm.upload_Program(program_file);
    vm.run();
}

