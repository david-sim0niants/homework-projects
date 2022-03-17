#ifndef __TURING_MACHINE_H__
#define __TURING_MACHINE_H__


#include <string>
#include <deque>
#include <vector>
#include <unordered_map>
#include <istream>



class TuringMachine_stream_parse_error : public std::runtime_error
{
public: TuringMachine_stream_parse_error(const std::string &msg) : std::runtime_error("Failed to parse a stream: " + msg) {}
};


class TuringMachine_exec_error : public std::runtime_error
{
public: TuringMachine_exec_error(const std::string &msg) : std::runtime_error(msg) {}
};


class TuringMachine
{

public:
    explicit TuringMachine(std::istream &is);

    struct Decision
    {
        char symbol;
        size_t state;
        char direction;
    };

    inline void decide(char symbol, size_t state, Decision &decision) const
    {
        auto found_symbol_it = symbol_set.find(symbol);
        if (found_symbol_it == symbol_set.end())
            throw TuringMachine_exec_error("Unexpected symbol.");
        decision = Decision(state_diagram[state * symbol_set.size() + found_symbol_it->second]);
    }


    const std::unordered_map<char, size_t> & get_SymbolSet()    const { return symbol_set;     }
    const std::vector<std::string>         & get_StateSet()     const { return state_set;      }
    const std::vector<Decision>            & get_StateDiagram() const { return state_diagram;  }

    void print_Info(std::ostream &os) const;

    void run_Tape(std::deque<char> &tape, std::deque<char>::iterator &position, size_t &state, size_t max_num_steps) const;

private:
    std::unordered_map<char, size_t> symbol_set;
    std::vector<std::string> state_set;
    std::vector<Decision> state_diagram;
};


void convert_StringTape_to_DequeTape(
    const std::string &tape_str, std::deque<char> &tape, size_t &start_pos_i, const std::unordered_map<char, size_t> &symbol_set
);


#endif