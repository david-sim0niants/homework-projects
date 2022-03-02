#include "dfa.h"

#include <string>
#include <regex>
#include <iostream>
#include <vector>


static void parse_Line(const std::string &line, DFA::ElementArray &elements)
{
    std::regex elem_regex("[^,]+");
    auto elems_begin = std::sregex_iterator(line.begin(), line.end(), elem_regex);
    auto elems_end = std::sregex_iterator();

    for (std::sregex_iterator it = elems_begin; it != elems_end; ++it)
    {
        std::smatch match = *it;
        std::string element = match.str();
        elements.push_back(element);
    }
}


DFA::DFA(std::istream &input)
{
    std::string curr_line;

    while (std::getline(input, curr_line) && curr_line.empty()) {}
    DFA::ElementArray input_alphabet;
    parse_Line(curr_line, input_alphabet);

    while (std::getline(input, curr_line) && curr_line.empty()) {}
    DFA::ElementArray output_alphabet;
    parse_Line(curr_line, output_alphabet);

    while (std::getline(input, curr_line) && curr_line.empty()) {}
    DFA::ElementArray states;
    parse_Line(curr_line, states);

    std::cout << "Found " << input_alphabet.size() << " input elements\n";
    for (size_t i = 0; i < input_alphabet.size(); ++i)
        std::cout << input_alphabet[i] << '\n';

    std::cout << "Found " << output_alphabet.size() << " output elements\n";
    for (size_t i = 0; i < output_alphabet.size(); ++i)
        std::cout << output_alphabet[i] << '\n';

    std::cout << "Found " << states.size() << " states\n";
    for (size_t i = 0; i < states.size(); ++i)
        std::cout << states[i] << '\n';
}
