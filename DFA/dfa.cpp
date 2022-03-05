#include "dfa.h"

#include <string>
#include <regex>
#include <iostream>
#include <vector>
#include <algorithm>



/**
 * @brief Remove leading and trailing whitespaces from string.
 */
static void trim_StringWhitespaces(std::string &str)
{
    auto no_space = [](unsigned char ch) { return !std::isspace(ch); };
    str.erase(str.begin(), std::find_if(str.begin(),  str.end(),  no_space));
    str.erase(std::find_if(str.rbegin(), str.rend(), no_space).base(), str.end());
}


/**
 * @brief Parse a string line into a set of elements.
 *
 * @param line The line.
 * @param elements The element set.
 * @param first_element Optional, if non null, will be set to the first element that appeared on the line
 * (in case of state set, we need to know the initial state, that's where this's used).
 */
static void parse_ElementSetLine(const std::string &line, DFA::ElementSet &elements, DFA::Element *first_element = nullptr)
{
    // Regex to split the line by commas.
    std::regex line_split_regex("[^,]+");
    auto elems_begin = std::sregex_iterator(line.begin(), line.end(), line_split_regex);
    auto elems_end = std::sregex_iterator();

    std::string elem_str;

    // set first element if needed
    if (first_element)
    {
        elem_str = elems_begin->str();
        trim_StringWhitespaces(elem_str);
        *first_element = elem_str;
    }

    // add trimmed elements
    size_t i = 0;
    for (std::sregex_iterator it = elems_begin; it != elems_end; ++it)
    {
        elem_str = it->str();
        trim_StringWhitespaces(elem_str);
        elements[elem_str] = i++; // an "index of its appearance in the line" is assigned as a value to the element key
    }
}


/**
 * @brief Parse function table that describes values for all possible arguments given to delta and lambda functions
 * (in this code, they're always paired).
 *
 * @param is Input stream to read and parse.
 * @param table The table to write to. Note, at this point the size of the table should be known and the table must be initialized.
 * @param output_set Output set (needs that only to make sure that unknown elements aren't present).
 * @param state_set State set (needs that only to make sure that unknown elements aren't present).
 */
static void parse_FuncTable(std::istream &is, DFA::FuncTable &table,
    const DFA::ElementSet &output_set, const DFA::ElementSet &state_set)
{
    // parse by |
    std::regex line_split_regex("[^\\|]+");
    // parse by ,
    std::regex pair_split_regex("[^\\,]+");

    std::string row;

    // iterate via rows
    for (size_t i = 0; i < table.get_Rows(); ++i)
    {
        while (std::getline(is, row) && row.empty()) {}
        if (row.empty())
            throw DFA_stream_parse_error("incomplete function table, expected more rows to appear.");

        // iterate via (output,state) or (b_n,q_n) pairs.
        auto pairs_it = std::sregex_iterator(row.begin(), row.end(), line_split_regex);
        auto pairs_end = std::sregex_iterator();

        for (size_t j = 0; j < table.get_Cols(); ++j, ++pairs_it)
        {
            if (pairs_it == pairs_end)
                throw DFA_stream_parse_error("incomplete function table, expected more columns to appear");

            // split current pair
            std::string pair_str = pairs_it->str();
            auto elem_it = std::sregex_iterator(pair_str.begin(), pair_str.end(), pair_split_regex);
            auto elem_end = std::sregex_iterator();

            DFA::Element output, state;
            if (elem_it == elem_end)
                throw DFA_stream_parse_error("incomplete function table, empty pair of output and state.");
            else output = elem_it->str();

            if (++elem_it == elem_end)
                throw DFA_stream_parse_error("incomplete function table, empty pair of output and state.");
            else state = elem_it->str();

            trim_StringWhitespaces(output);
            if (output_set.find(output) == output_set.end())
                throw DFA_stream_parse_error("found an output element (which is '"+ output+"'}) that isn't included in the output set.");

            trim_StringWhitespaces(state);
            if (state_set.find(state) == state_set.end())
                throw DFA_stream_parse_error("found an state element (which is '" + state + "') that isn't included in the state set.");

            // here finally sure that output and state are valid
            table.set_Cell(i, j, std::move(output), std::move(state));
        }
    }
}


DFA::DFA(std::istream &is)
{
    std::string curr_line;

    // Parse line of input set.
    while (std::getline(is, curr_line) && curr_line.empty()) {}
    parse_ElementSetLine(curr_line, input_set);

    // Parse line of output set.
    while (std::getline(is, curr_line) && curr_line.empty()) {}
    parse_ElementSetLine(curr_line, output_set);

    // Parse line containing state_set
    while (std::getline(is, curr_line) && curr_line.empty()) {}
    parse_ElementSetLine(curr_line, state_set, &default_state);

    // Just to get sure everything is ok.
    std::cout << "Found " << input_set.size() << " input elements\n";
    for (auto &&element_index_pair : input_set)
    {
        std::cout << element_index_pair.first << ", ";
    }

    std::cout << "\nFound " << output_set.size() << " output elements\n";
    for (auto &&element_index_pair : output_set)
        std::cout << element_index_pair.first << ", ";

    std::cout << "\nFound " << state_set.size() << " state elements\n";
    for (auto &&element_index_pair : state_set)
        std::cout << element_index_pair.first << ", ";
    std::cout << '\n';

    // Initialize function table.
    function_table = FuncTable(state_set.size(), input_set.size());
    parse_FuncTable(is, function_table, output_set, state_set);
}


DFA::DFA(ElementSet &&input_set, ElementSet &&output_set, ElementSet &&state_set, FuncTable &&function_table)
    : input_set(input_set), output_set(output_set), state_set(state_set), function_table(function_table) {}


bool DFA::run_Stream(std::istream &input, std::ostream &output, Element &last_state, bool show_state)
{
    return run_Stream(input, output, default_state, last_state, show_state);
}


bool DFA::run_Stream(std::istream &input, std::ostream &output, const Element &initial_state, Element &last_state, bool show_state)
{
    // get single line, interpret enter as a sign to run the inserted tape
    std::string line;
    std::getline(input, line);
    if (line.empty()) return false;

    // split by commas
    std::regex line_split_regex("[^,]+");
    auto input_begin = std::sregex_iterator(line.begin(), line.end(), line_split_regex);
    auto input_end = std::sregex_iterator();

    Element curr_input, curr_state = initial_state, curr_output;

    for (std::sregex_iterator it = input_begin; it != input_end; ++it)
    {
        curr_input = it->str();
        trim_StringWhitespaces(curr_input);

        if (input_set.find(curr_input) == input_set.end())
            throw DFA_stream_parse_error("input element not in input set.");

        // evaluate for single input
        eval(curr_input, curr_state, curr_output);

        // output either with state or without
        if (show_state)
            output << '(' << curr_output << ',' << curr_state << "), ";
        else output << curr_output << ", ";
    }
    // assign last state
    last_state = curr_state;
    return true;
}
