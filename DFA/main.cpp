#include <fstream>
#include <iostream>
#include <algorithm>
#include <string.h>

#include "dfa.h"



/**
 * @brief Struct containing all possible arguments that can be parsed from a command.
 */
struct ParsedArgs
{
    bool program_fn_present = false;
    std::string program_fn;

    bool input_fn_present = false;
    std::string input_fn;

    bool output_fn_present = false;
    std::string output_fn;

    bool show_state = false;
    bool reset_newline = false;
    bool help = false;
};


/**
 * @brief Find optional argument that usually starts with dash or double dash. Returns true on success, false otherwise.
 *
 * @param prefixes Possible dashed prefixes for the argument.
 */
bool find_OptionalArg(int argc, char *argv[], const std::vector<std::string> &prefixes, std::string &found_arg)
{
    // are prefix and argument separate ?
    bool separate;
    // find prefix
    auto found_prefix_it = std::find_if(argv + 1, argv + argc, [&prefixes, &separate, &found_arg](char *arg)
    {
        for (size_t i = 0; i < prefixes.size(); ++i)
        {
            auto &prefix = prefixes[i];
            if (strstr(arg, prefix.c_str()) == arg)
            {
                if (strlen(arg) > prefix.size())
                {
                    found_arg = std::string(arg + prefix.size());
                    separate = false;
                }
                else
                    separate = true;
                return true;
            }
        }
        return false;
    });
    if (found_prefix_it == argc + argv) // prefix wasn't found
        return false;
    if (separate) // prefix is separate from the argument
    {
        auto found_arg_it = ++found_prefix_it;
        if (found_arg_it == argc + argv)
            return false;
        else
        {
            found_arg = *found_arg_it;
            return true;
        }
    }
    // prefix wasn't separate and the argument is already found here.
    return true;
}


/**
 * @brief Parse command arguments.
 *
 * @param parsed_args
 */
void parse_CommandArgs(int argc, char *argv[], ParsedArgs &parsed_args)
{
    const char *help_prefixes[2] = {"-h", "--help"};
    parsed_args.help = std::find_if(argv + 1, argv + argc, [&help_prefixes](char *arg)
    {
        return !(strcmp(arg, help_prefixes[0]) && strcmp(arg, help_prefixes[1]));
    }) != (argv + argc);
    if (parsed_args.help)
        return;

    parsed_args.program_fn_present = argc > 1;
    if (argc > 1)
        parsed_args.program_fn = argv[1];
    else return;

    const char *show_state_prefixes[2] = {"-s", "--show-state"};
    parsed_args.show_state = std::find_if(argv + 1, argv + argc, [&show_state_prefixes](char *arg)
    {
        return !(strcmp(arg, show_state_prefixes[0]) && strcmp(arg, show_state_prefixes[1]));
    }) != (argv + argc);

    const char *reset_newline_prefixes[2] = {"-r", "--reset-newline"};
    parsed_args.reset_newline = std::find_if(argv + 1, argv + argc, [&reset_newline_prefixes](char *arg)
    {
        return !(strcmp(arg, reset_newline_prefixes[0]) && strcmp(arg, reset_newline_prefixes[1]));
    }) != (argv + argc);

    parsed_args.input_fn_present = find_OptionalArg(argc, argv, {"-I"}, parsed_args.input_fn);
    parsed_args.output_fn_present = find_OptionalArg(argc, argv, {"-O"}, parsed_args.output_fn);
}



int main(int argc, char *argv[])
{
    try
    {
        // parse arguments
        ParsedArgs parsed_args;
        parse_CommandArgs(argc, argv, parsed_args);

        if (parsed_args.help)
        {
            // display help
            std::cout <<
R""""(Usage: dfa program_filename [-s --show-state] [-r --reset-newline] [-I] [-O]
Options:
    -s, --show-state        Display states with corresponding outputs like (b_n, q_n).
    -r, --reset-newline     Reset DFA's state on every new line.
    -I                      Input from file. -I path/to/input/tape
    -O                      Output to file.  -O /path/to/output/tape

If no path to program filename given, will use stdin.
)"""";
            return 0;
        }


        std::ifstream program_file;
        if (parsed_args.program_fn_present)
        {
            // open program file
            program_file = std::ifstream(parsed_args.program_fn);
            if (!program_file)
            {
                std::cerr << "Error: couldn't open file \"" << parsed_args.program_fn << "\"\n\n";
                std::terminate();
            }
        }
        // start program stream
        std::istream &program_stream = parsed_args.program_fn_present ? program_file : std::cin;


        std::ifstream input_file;
        if (parsed_args.input_fn_present)
        {
            // open input file
            input_file = std::ifstream(parsed_args.input_fn);
            if (!input_file)
            {
                std::cerr << "Error: couldn't open file \"" << parsed_args.input_fn << "\"\n\n";
                std::terminate();
            }
        }
        // start input stream
        std::istream &input_stream = parsed_args.input_fn_present ? input_file : std::cin;
        bool interactive_input_mode = !parsed_args.input_fn_present; // enable interactive input mode if no input file given


        std::ofstream output_file;
        if (parsed_args.output_fn_present)
        {
            // open output file
            output_file = std::ofstream(parsed_args.output_fn);
            if (!output_file)
            {
                std::cerr << "Error: couldn't open file \"" << parsed_args.output_fn << "\"\n\n";
                std::terminate();
            }
        }
        // start output stream
        std::ostream &output_stream = parsed_args.output_fn_present ? output_file : std::cout;


        // create DFA from program_stream
        DFA dfa(program_stream);
        std::cout << '\n';
        DFA::Element state = dfa.get_DefaultState(); // acquire default state
        while (input_stream)
        {
            try
            {
                if (interactive_input_mode)
                    std::cout << ">>> "; // print >>> like in python

                // in order not to get messy, output new line if inputted new line wasn't empty.
                if (dfa.run_Stream(input_stream, output_stream, state, state, parsed_args.show_state))
                    output_stream << '\n';
            }
            catch (DFA_stream_parse_error &e)
            {
                std::cerr << e.what() << '\n';
            }

            if (parsed_args.reset_newline)
                state = dfa.get_DefaultState();
        }
    }
    catch (DFA_stream_parse_error &e)
    {
        std::cerr << e.what() << '\n';
    }

    return 0;
}
