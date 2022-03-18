#include "turing_machine.h"
#include <sstream>
#include <algorithm>
#include <unordered_map>


/**
 * @brief Remove leading and trailing whitespaces from string.
 */
static void trim_StringWhitespaces(std::string &str)
{
    auto no_space = [](unsigned char ch) { return !std::isspace(ch); };
    str.erase(str.begin(), std::find_if(str.begin(),  str.end(),  no_space));
    str.erase(std::find_if(str.rbegin(), str.rend(), no_space).base(), str.end());
}


TuringMachine::TuringMachine(std::istream &is)
{
    std::string line;

    while (std::getline(is, line) && line.empty()) {}
    if (line.empty())
        throw TuringMachine_stream_parse_error("Empty file, no symbol set could be found.");

    // read the line containing symbol set
    size_t symbol_i = 0;
    for (char symbol : line)
    {
        // whitespaces, commas and vertical bars aren't allowed and are used as seperators
        // also duplicate symbols are ignored
        if (symbol != ',' && !std::iswspace(symbol) && symbol != '|' && symbol_set.find(symbol) == symbol_set.end())
        {
            symbol_set.emplace(symbol, symbol_i++);
            if (symbol_set.size() == 1)
                default_symbol = symbol; // set the first symbol as the default symbol
        }
    }


    // read the line containing state names (that are strings)

    // temporary unordered map that maps state names to their indices
    std::unordered_map<std::string, size_t> state_to_index_map;
    /* Note that the states are indexed by the order of their appearance and their indices are used instead of string names because
    indices are more lightweight and the states aren't not assigned or read by the outside world. However, their string names are
    saved for demonstration and debug purposes. */
    size_t state_symbol_i = 0;

    while (std::getline(is, line) && line.empty()) {}
    if (line.empty())
        throw TuringMachine_stream_parse_error("No state set could be found.");

    std::istringstream line_ss(line);
    std::string state;
    while (std::getline(line_ss, state, ','))
    {
        trim_StringWhitespaces(state);
        if (state_to_index_map.find(state) == state_to_index_map.end())
        {
            state_to_index_map.emplace(state, state_symbol_i++);
            state_set.push_back(state);
        }
    }


    // read lines containing state diagram
    // cell format - symbol,state,direction where direction is either l (left) or r (right).

    size_t num_symbols = symbol_set.size();
    size_t num_states = state_set.size();

    state_diagram.resize(num_symbols * num_states);

    size_t decision_i = 0;

    // read row
    while (std::getline(is, line))
    {
        line_ss.clear();
        line_ss.str(line);

        // read cell
        std::string decision_str;
        while (std::getline(line_ss, decision_str, '|') && !decision_str.empty())
        {
            std::istringstream decision_ss(decision_str);

            // read 3 'words' in the cell
            std::string decision_words[3] = {};
            for (size_t i = 0; i < 3; ++i)
            {
                std::string &word = decision_words[i];
                while (std::getline(decision_ss, word, ',') && word.empty()) {}

                trim_StringWhitespaces(word);
                if (word.empty())
                    throw TuringMachine_stream_parse_error("Found an incomplete decision in the state diagram.");
            }

            char symbol = decision_words[0][0];
            if (symbol_set.find(symbol) == symbol_set.end())
                throw TuringMachine_stream_parse_error("Found a symbol in the state diagram that wasn't included in the symbol set.");

            std::string &state = decision_words[1];
            auto found_state_it = state_to_index_map.find(state);
            if (found_state_it == state_to_index_map.end())
                throw TuringMachine_stream_parse_error("Found a state in the state diagram that wasn't included in the state set.");
            size_t state_index = found_state_it->second;

            char direction = decision_words[2][0];
            if (direction == 'l')
                direction = -1;
            else if (direction == 'r')
                direction = +1;
            else
                throw TuringMachine_stream_parse_error("The character indicating a direction in the state diagram must be either 'l'\
                or 'r' respectively 'left' or 'right'.");

            state_diagram[decision_i++] = { symbol, state_index, direction };
        }
    }

    if (decision_i < state_diagram.size())
        throw TuringMachine_stream_parse_error("Incomplete state diagram. Not the all cells were present.");
}


void TuringMachine::print_Info(std::ostream &os) const
{
    os << "Symbol set: ";
    for (auto &&symbol : symbol_set)
        os << symbol.first << ',';
    os << "\nState set: ";
    for (auto &&state : state_set)
        os << state << ',';
    os << "\nState diagram: \n";

    size_t decision_i = 0;
    for (size_t i = 0; i < state_set.size(); ++i)
    {
        for (size_t j = 0; j < symbol_set.size(); ++j)
        {
            const Decision &decision = state_diagram[decision_i++];
            os << decision.symbol << ',' << state_set[decision.state] << ',' << (decision.direction > 0 ? 'r' : 'l');
            if (j + 1 < symbol_set.size())
                os << " | ";
            else
                os << '\n';
        }
    }
}


void TuringMachine::exec_Tape(std::deque<char> &tape, std::deque<char>::iterator &head, size_t &state, size_t max_num_steps) const
{
    // halt state is the last state
    size_t halt_state = state_set.size() - 1;

    if (tape.empty())
    {
        state = halt_state;
        return;
    }
    if (head == tape.end())
        throw TuringMachine_exec_error("Head pointing at one cell past the end of tape");
    if (head == tape.begin() - 1)
        throw TuringMachine_exec_error("Head pointing at one cell before the start of tape");

    bool wait_halt = max_num_steps == 0; // run without limiting number of steps ?
    size_t step = 0;

    while (state != halt_state && (wait_halt || step < max_num_steps))
    {
        Decision decision;
        decide(*head, state, decision);

        *head = decision.symbol; // assign new head
        state = decision.state; // assign new state

        // go left or right
        if (decision.direction > 0)
        {
            // push back of the tape if already at the end of the tape and must go to right
            if (head == tape.end() - 1)
            {
                tape.push_back(default_symbol);
                head = tape.end() - 1;
            }
            else
            {
                ++head;
            }
        }
        else
        {
            // push front of the tape if already at the beginning of the tape and must go to left
            if (head == tape.begin())
            {
                tape.push_front(default_symbol);
                head = tape.begin();
            }
            else
            {
                --head;
            }
        }
        ++step;
    }
}