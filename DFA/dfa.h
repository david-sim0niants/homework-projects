#ifndef __DETERMINSTIC_FINITE_AUTOMATA__
#define __DETERMINSTIC_FINITE_AUTOMATA__


#include <vector>
#include <istream>


/**
 * @brief Deterministic finite automata simulator.
 **/
class DFA
{
public:
    // Shorthand type for string represented element arrays.
    using ElementArray = std::vector<std::string>;

    explicit DFA(std::istream &input);
    DFA(ElementArray &&input_alphabet, ElementArray &&output_alphabet, ElementArray &&states);

private:
    // Set of all possible input elements.
    ElementArray input_alphabet;
    // Set of all possible output elements.
    ElementArray output_alphabet;
    // Set of all possible states
    ElementArray states;
};


#endif
