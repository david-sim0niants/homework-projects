#include "turing_machine.h"
#include <iostream>
#include <fstream>


int main(int argc, const char *argv[])
{
    bool read_from_file = false;
    std::ifstream file;

    bool debug_mode = false;

    if (argc > 1)
    {
        file = std::ifstream(argv[1]);
        if (!file)
            std::cerr << "Error couldn't open file " << argv[1] << ". Using stdin.\n";
        else
            read_from_file = true;

        if (argc > 2)
        {
            debug_mode = argv[2][0] == 'd';
        }
    }

    std::istream &is = read_from_file ? file : std::cin;
    TuringMachine turing_machine(is);

    turing_machine.print_Info(std::cout);

    while (std::cin)
    {
        std::string tape_str;
        std::getline(std::cin, tape_str);

        std::deque<char> tape;
        size_t start_pos_i = 0;
        convert_StringTape_to_DequeTape(tape_str, tape, start_pos_i, turing_machine.get_SymbolSet());
        std::deque<char>::iterator pos = tape.begin() + start_pos_i;

        size_t state = 0;
        if (debug_mode)
        {
            size_t halt_state = turing_machine.get_StateSet().size();
            while (state != halt_state)
            {
                turing_machine.run_Tape(tape, pos, state, 1);
                std::cout << "DEBUG:Tape>";
                for (char symbol : tape)
                {
                    std::cout << symbol;
                }
                std::cout << std::endl;
            }
        }
        else
        {
            turing_machine.run_Tape(tape, pos, state, 0);
            std::cout << "Tape>";
            for (char symbol : tape)
            {
                std::cout << symbol;
            }
            std::cout << std::endl;
        }
    }
}