#ifndef __TURING_MACHINE_H__
#define __TURING_MACHINE_H__


#include <string>
#include <deque>
#include <vector>
#include <map>
#include <istream>



class TuringMachine_stream_parse_error : public std::runtime_error
{
public: TuringMachine_stream_parse_error(const std::string &msg) : std::runtime_error("Failed to parse a stream: " + msg) {}
};


class TuringMachine_exec_error : public std::runtime_error
{
public: TuringMachine_exec_error(const std::string &msg) : std::runtime_error(msg) {}
};


/**
 * @brief Turing machine simulator.
 */
class TuringMachine
{
public:
    /**
     * @brief Initialize from input stream.
     */
    explicit TuringMachine(std::istream &is);

    /**
     * @brief Decision type consisting of symbol to output on current cell of the tape, the state to assign and the direction
     * to move to the neighboring cell (left or right correspondingly -1 or 1).
     */
    struct Decision
    {
        char symbol;
        size_t state;
        char direction;
    };

    /**
     * @brief Make a decision for the current symbol and state.
     */
    inline void decide(char symbol, size_t state, Decision &decision) const
    {
        auto found_symbol_it = symbol_set.find(symbol);
        if (found_symbol_it == symbol_set.end())
            throw TuringMachine_exec_error("Unexpected symbol.");
        decision = state_diagram[state * symbol_set.size() + found_symbol_it->second];
    }


    // basic getters

    const std::map<char, size_t> & get_SymbolSet()    const { return symbol_set;     }
    const std::vector<std::string>         & get_StateSet()     const { return state_set;      }
    const std::vector<Decision>            & get_StateDiagram() const { return state_diagram;  }

    /**
     * @brief Print info about the machine on a given output stream.
     */
    void print_Info(std::ostream &os) const;
    /**
     * @brief Execute Turing machine on a given tape.
     *
     * @param tape Doubly ended queue is chosen because it's optimized for both front and back insertions.
     * @param head Position of the head on the tape.
     * @param state State to start with. At the end will be set to the last state in the state set which's the HALT state.
     * @param max_num_steps Max number of steps to execute if the machine doesn't reach HALT state. 0 for infinite amount of states.
     * This can be and is used for debug purposes.
     */
    void exec_Tape(std::deque<char> &tape, std::deque<char>::iterator &head, size_t &state, size_t max_num_steps) const;

private:
    /**
     * @brief Symbol set which is actually a map mapping char symbols to their corresponding indices in the state diagram.
     */
    std::map<char, size_t> symbol_set;
    /**
     * @brief String names of the indexed states.
     */
    std::vector<std::string> state_set;
    /**
     * @brief State diagram that's vector holding Decision-s for every symbol on tape and internal state.
     */
    std::vector<Decision> state_diagram;
    /**
     * @brief Default symbol by which the unassigned cells on tape should be filled.
     * It's selected as the first symbol appearing on the input stream in the main constructor.
     */
    char default_symbol;
};


#endif