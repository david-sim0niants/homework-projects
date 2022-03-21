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
        // find file
        file = std::ifstream(argv[1]);
        if (!file)
            std::cerr << "Error couldn't open file " << argv[1] << ". Using stdin.\n";
        else
            read_from_file = true;

        if (argc > 2)
        {
            // enable debug mode
            debug_mode = argv[2][0] == 'd';
        }
    }

    std::istream &is = read_from_file ? file : std::cin;
    TuringMachine turing_machine(is);

    if (turing_machine.get_StateSet().size() <= (1 << 17) + 1)
        turing_machine.print_Info(std::cout);

    std::cout << "INPUT:Tape>";
    while (std::cin)
    {
        std::string tape_str;
        std::getline(std::cin, tape_str);

        std::deque<char> tape(tape_str.size());
        std::copy(tape_str.begin(), tape_str.end(), tape.begin());

        std::deque<char>::iterator head = tape.begin();

        size_t state = 0;

        try
        {
            if (debug_mode)
            {
                size_t halt_state = turing_machine.get_StateSet().size() - 1;
                while (state != halt_state)
                {
                    turing_machine.exec_Tape(tape, head, state, 1); // execute single step
                    size_t head_pos = std::distance(tape.begin(), head); // find head position
                    size_t symbol_i = 0;

                    std::cout << "DEBUG:Tape>";
                    for (char symbol : tape)
                    {
                        if (symbol_i == head_pos)
                            std::cout << '|'; // print vertical bar before the symbol pointed by the head
                        std::cout << symbol;
                        ++symbol_i;
                    }
                    std::cout << " State:" << turing_machine.get_StateSet()[state] << " Head:" << head_pos << std::endl;
                }
            }
            else
            {
                turing_machine.exec_Tape(tape, head, state, 0);
                std::cout << "FINAL:Tape>";
                for (char symbol : tape)
                {
                    std::cout << symbol;
                }
                std::cout << std::endl;
            }
        }
        catch (const TuringMachine_exec_error &e)
        {
            std::cerr << "Error: " << e.what() << '\n';
        }
        std::cout << "INPUT:Tape>";
    }
}