#ifndef __TURING_MACHINE_H__
#define __TURING_MACHINE_H__


#include <string>
#include <deque>
#include <vector>
#include <unordered_map>
#include <istream>


class TuringMachine
{
    std::unordered_map<char, size_t> symbol_set;
    std::vector<std::string> state_set;

    struct Decision
    {
        char symbol;
        size_t state;
        char direction;
    };
    std::vector<Decision> state_diagram;

public:
    explicit TuringMachine(std::istream &is);


    void decide(char symbol, size_t state, Decision &decision)
    {
        size_t symbol_index = symbol_set[symbol];
        decision = Decision(state_diagram[state * state_set.size() + symbol_index]);
    }


    void print_Info(std::ostream &os);
};


class TuringMachine_stream_parse_error : public std::runtime_error
{
public: TuringMachine_stream_parse_error(const std::string &msg) : std::runtime_error("Failed to parse a stream: " + msg) {}
};


#endif