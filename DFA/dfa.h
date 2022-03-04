#ifndef __DETERMINSTIC_FINITE_AUTOMATA__
#define __DETERMINSTIC_FINITE_AUTOMATA__


#include <string>
#include <vector>
#include <istream>
#include <map>


/**
 * @brief Deterministic finite automata simulator.
 **/
class DFA
{
public:
    // Element type
    using Element = std::string;
    /**
     * @brief Mapping from element to its index. This mostly relates to input and state elements as this mapping
    holds indices of them in the function table.
     */
    using ElementSet = std::map<Element, size_t>;

    /**
     * @brief Function table that holds all the values for every argument that lambda and delta functions may receive.
     **/
    class FuncTable
    {
        // (b_n,q_n) pairs as n*m*2 sized vector.
        std::vector<Element> output_state_pairs;
        size_t rows, cols;

    public:
        FuncTable() : rows(0), cols(0) {}
        FuncTable(size_t rows, size_t cols) : rows(rows), cols(cols), output_state_pairs(rows * cols * 2) {}

        void get_Cell(size_t i, size_t j, std::string &output, std::string &state)
        {
            auto main_i = (i * cols + j) * 2;
            output = output_state_pairs[main_i];
            state = output_state_pairs[main_i + 1];
        }

        void set_Cell(size_t i, size_t j, std::string &&output, std::string &&state)
        {
            auto main_i = (i * cols + j) * 2;
            output_state_pairs[main_i] = std::move(output);
            output_state_pairs[main_i + 1] = std::move(state);
        }

        inline size_t get_Rows() { return rows; }
        inline size_t get_Cols() { return cols; }
    };

    DFA() = default;
    /**
     * @brief Create a dfa from an input stream.
     * */
    explicit DFA(std::istream &is);
    /**
     * @brief Create a dfa by parts.
     * */
    DFA(ElementSet &&input_set, ElementSet &&output_set, ElementSet &&state_set, FuncTable &&func_table);


    /**
     * @brief Run on single input.
     *
     * @param input Input element.
     * @param state State element.
     * @param output Output element.
     */
    void eval(const Element &input, Element &state, Element &output)
    {
        function_table.get_Cell(state_set[state], input_set[input], output, state);
    }

    /**
     * @brief Run input, assign to output.
     */
    template<typename InIter_T, typename OutIter_T>
    void run(InIter_T src_begin, InIter_T src_end, OutIter_T dst_begin)
    {
        auto out_it = dst_begin;
        Element state = initial_state;
        for (auto it = src_begin; it != src_end; ++it, ++out_it)
        {
            eval(*it, state, *out_it);
        }
    }

    /**
     * @brief Run input stream and pipe result to output stream.
     *
     * @param input Input stream.
     * @param output Output stream.
     * @param show_state If true, will show (output, state) pairs instead of just output.
     */
    void run_Stream(std::istream &input, std::ostream &output, bool show_state = false);

private:
    // Set of all possible input elements.
    ElementSet input_set;
    // Set of all possible output elements.
    ElementSet output_set;
    // Set of all possible state_set
    ElementSet state_set;
    // Where to start from.
    Element initial_state;
    // Function table
    FuncTable function_table;
};


/**
 * @brief Error to throw when failing to parse an input stream into DFA.
 */
class DFA_stream_parse_error : public std::runtime_error
{
public:
    explicit DFA_stream_parse_error(const std::string &err) : std::runtime_error("Failed to parse a stream: " + err) {}
};



#endif
