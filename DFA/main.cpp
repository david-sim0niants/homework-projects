#include <fstream>
#include <iostream>

#include "dfa.h"

using namespace std;

int main(int argc, char *argv[])
{
    std::ifstream file(argv[1]);
    DFA dfa(file);

    return 0;
}
