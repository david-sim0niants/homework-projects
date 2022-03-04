#include <fstream>
#include <iostream>

#include "dfa.h"



int main(int argc, char *argv[])
{
    try
    {
        DFA dfa;
        if (argc > 1)
        {
            std::ifstream file(argv[1]);
            if (file)
                dfa = DFA(file);
            else
            {
                std::cerr << "Error: couldn't open file " << argv[1] << ".\n";
                std::terminate();
            }
        }
        else
            dfa = DFA(std::cin);

        while (true)
        {
            try
            {
                dfa.run_Stream(std::cin, std::cout);
            }
            catch (DFA_stream_parse_error &e)
            {
                std::cerr << e.what() << '\n';
            }
            std::cout << '\n';
        }
    }
    catch (DFA_stream_parse_error &e)
    {
        std::cerr << e.what() << '\n';
    }

    return 0;
}
