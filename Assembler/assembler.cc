#include "assembler.h"

#include <limits>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <optional>


// Some constant definitions of the assembly language
class AssemblyDef
{
public:
    // sign that must be put before immediates to differentiate them from addresses like #1 #2 #3
    char immediate_sign = '#';
    // delimiter that can be put between operands and nowhere else
    char delimiter = ',';
    // sign that starts a comment
    const char *comment = "//";
    // mnemonic string to mnemonic itself
    std::unordered_map<std::string, Mnemonic> mnemonics = {
        {"ADD", Mnemonic::ADD}, {"add", Mnemonic::ADD},
        {"SUB", Mnemonic::SUB}, {"sub", Mnemonic::SUB},
        {"OR",  Mnemonic::OR }, {"or",  Mnemonic::OR },
        {"NOT", Mnemonic::NOT}, {"not", Mnemonic::NOT},
        {"AND", Mnemonic::AND}, {"and", Mnemonic::AND},
        {"XOR", Mnemonic::XOR}, {"xor", Mnemonic::XOR},
        {"MUL", Mnemonic::MUL}, {"mul", Mnemonic::MUL},
        {"JE",  Mnemonic::JE }, {"je",  Mnemonic::JE },
        {"JNE", Mnemonic::JNE}, {"jne", Mnemonic::JNE},
        {"JLT", Mnemonic::JLT}, {"jlt", Mnemonic::JLT},
        {"JLE", Mnemonic::JLE}, {"jle", Mnemonic::JLE},
        {"JGT", Mnemonic::JGT}, {"jgt", Mnemonic::JGT},
        {"JGE", Mnemonic::JGE}, {"jge", Mnemonic::JGE},
        {"JMP", Mnemonic::JMP}, {"jmp", Mnemonic::JMP},
        {"MOV", Mnemonic::MOV}, {"mov", Mnemonic::MOV},
        {"NOP", Mnemonic::NOP}, {"nop", Mnemonic::NOP},
    };
    // register name to its index mapping
    std::unordered_map<std::string, unsigned int> registers;

    // binary definitions
    static constexpr unsigned char CONDITIONAL_BIT = 0x20;
    static constexpr unsigned char FIRST_IMMEDIATE_BIT = 0x40;
    static constexpr unsigned char SECOND_IMMEDIATE_BIT = 0x80;

    // mnemonic to opcode mappings
    std::unordered_map<Mnemonic, char> opcodes = {
        {Mnemonic::ADD, 0},
        {Mnemonic::SUB, 1},
        {Mnemonic::AND, 2},
        {Mnemonic::OR,  3},
        {Mnemonic::NOT, 4},
        {Mnemonic::XOR, 5},
        {Mnemonic::MUL, 6},
        {Mnemonic::JE,  0 | CONDITIONAL_BIT},
        {Mnemonic::JNE, 1 | CONDITIONAL_BIT},
        {Mnemonic::JLT, 2 | CONDITIONAL_BIT},
        {Mnemonic::JLE, 3 | CONDITIONAL_BIT},
        {Mnemonic::JGT, 4 | CONDITIONAL_BIT},
        {Mnemonic::JGE, 5 | CONDITIONAL_BIT},
        {Mnemonic::JMP, 0 | CONDITIONAL_BIT | FIRST_IMMEDIATE_BIT | SECOND_IMMEDIATE_BIT},
        {Mnemonic::MOV, 3 | SECOND_IMMEDIATE_BIT},
        {Mnemonic::NOP, 3}
    };

    // singleton
    static const AssemblyDef instance;
    // max mnemonic name length
    static constexpr unsigned int MAX_MNM_LEN = 3;
    // max register name length
    static constexpr unsigned int MAX_REG_LEN = 3;

private:
    AssemblyDef()
    {
        // register map initializer
        for (unsigned int i = 0; i < NUM_REGISTERS; ++i)
        {
            std::string i_str = std::to_string(i);
            registers['r' + i_str] = i;
            registers['R' + i_str] = i;
        }

        registers["io"] = IO_REGISTER_INDEX;
        registers["IO"] = IO_REGISTER_INDEX;

        registers["pc"] = COUNTER_INDEX;
        registers["PC"] = COUNTER_INDEX;
    }
};

// singleton definition
inline const AssemblyDef AssemblyDef::instance;


// helper function to check if a variant is in a monostate (has no value)
template<typename... Types>
static bool is_in_Monostate(std::variant<Types...> variant)
{
    return std::holds_alternative<std::monostate>(variant);
}


// holds data and references to data needed for parsing functions
struct AssemblyParseState
{
    // next instruction address
    uint32_t next_instr_addr = 0;
    // ref to label map
    Label2Int_Map &labels;
    // last label iterator added to the map
    Label2Int_Map::iterator last_defined_label_it = {};
    // assembler messages
    std::vector<std::string> &messages;
};


// parse number
template<typename T>
static std::optional<T> parse_Number(std::istream &input)
{
    auto initial_pos = input.tellg();

    enum NumSystem { DEC, OCT, HEX } num_system = DEC;

    char chars_left_read = 2;

    while (input && chars_left_read > 0)
    {
        char c;
        input >> c;
        --chars_left_read;

        if (std::iswspace(c))
        {
            continue;
        }

        if (chars_left_read == 1)
        {
            if (c == '0')
            {
                continue;
            }

            input.seekg(-1, std::ios::cur);
            break;
        }
        else if (c == 'x') // if seen x after 0, it's hex
        {
            num_system = HEX;
        }
        else if ('0' <= c && c <= '9') // if seen digit after 0, it's oct, go back by 1
        {
            num_system = OCT;
            input.seekg(-1, std::ios::cur);
        }
        else // if seen another character, go back by 2, the number is just 0
        {
            input.seekg(-2, std::ios::cur);
        }
    }

    switch (num_system)
    {
    case DEC:
        input >> std::dec;
        break;
    case OCT:
        input >> std::oct;
        break;
    case HEX:
        input >> std::hex;
        break;
    default:
        break;
    }

    if (!input)
    {
        input.seekg(initial_pos, std::ios::beg);
        return {};
    }

    T number;
    input >> number;

    if (input.fail())
    {
        input.clear(); // clear fail bits from stream
        input.seekg(initial_pos, std::ios::beg);
        return {};
    }

    if (input)
    {
        char c;
        input >> c;

        // whitespace or delimiter checks
        // note this pattern repeats a lot
        if (!std::iswspace(c) && c != AssemblyDef::instance.delimiter)
        {
            input.seekg(initial_pos, std::ios::beg);
            return {};
        }
    }

    return number;
}


// parse immediate
static std::optional<OperandImmediate> parse_Immediate(std::istream &input)
{
    while (input)
    {
        char c;
        input >> c;

        if (std::iswspace(c))
        {
            continue;
        }

        if (c != AssemblyDef::instance.immediate_sign)
        {
            // go to the previous position in file if failed
            // note this pattern repeats a lot
            input.seekg(-1, std::ios::cur);
            return {};
        }

        break;
    }

    return parse_Number<OperandImmediate>(input);
}


static bool is_Alpha(char c)
{
    return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
}

static bool is_Digit(char c)
{
    return '0' <= c && c <= '9';
}


static std::string parse_Identifier(std::istream &input)
{
    int chars_read = 0;
    std::string identifier;

    while (input)
    {
        char c;
        input >> c;

        if (identifier.empty())
        {
            if (std::iswspace(c))
            {
                continue;
            }

            if (!is_Alpha(c) && c != '.' && c != '_')
            {
                break;
            }

            identifier.push_back(c);
        }
        else
        {
            if (std::iswspace(c) || !is_Alpha(c) && !is_Digit(c) && c != '.' && c != '_')
            {
                break;
            }

            identifier.push_back(c);
        }

    }

    if (identifier.empty())
    {
        // go chars_read + 1 back if failed to parse an identifier
        input.seekg(-chars_read - 1, std::ios::cur);
    }
    else
    {
        // go -1 back if identifier could be parsed
        input.seekg(-1, std::ios::cur);
    }

    return identifier;
}


// check if an identifier is a reserved keyword in assembly, it can be a mnemonic or register name
static bool check_if_Reserved(std::string &identifier)
{
    return !
    (AssemblyDef::instance.mnemonics.find(identifier) == AssemblyDef::instance.mnemonics.end()
    &&
    AssemblyDef::instance.registers.find(identifier) == AssemblyDef::instance.registers.end());
}


// parse label definition
static bool parse_LabelDef(std::istream &input, AssemblyParseState &parse_state)
{
    auto initial_pos = input.tellg();
    parse_state.last_defined_label_it = {};

    std::string label = parse_Identifier(input);

    if (label.empty())
    {
        input.seekg(initial_pos, std::ios::beg);
        return false;
    }

    int chars_read = 0;
    while (input)
    {
        char c;
        input >> c;

        if (std::iswspace(c))
        {
            chars_read = 0;
            continue;
        }

        if (c != ':')
        {
            break;
        }

        if (parse_state.labels.find(label) != parse_state.labels.end())
        {
            parse_state.messages.push_back("Label already defined.");
            return false;
        }

        if (check_if_Reserved(label))
        {
            parse_state.messages.push_back("Token " + label + " is reserved and can't be a label name.");
            return false;
        }

        parse_state.last_defined_label_it = parse_state.labels.insert(
            std::make_pair(label, parse_state.next_instr_addr)
        ).first;
        return true;
    }

    input.seekg(initial_pos, std::ios::beg);
    return false;
}


// parse mnemonic
static Mnemonic parse_Mnemonic(std::istream &input)
{
    auto initial_pos = input.tellg();

    // probably the only parser function that temporary enables whitespace skips
    // to parse a token that can't have various signs like commas, colons...
    std::string token;
    input.setf(std::ios::skipws);
    input >> token;
    input.unsetf(std::ios::skipws); // disable whitespace skips

    auto &mnemonics = AssemblyDef::instance.mnemonics;

    auto found_it = mnemonics.find(token);
    if (found_it != mnemonics.end())
    {
        return found_it->second;
    }
    else
    {
        input.seekg(initial_pos, std::ios::beg);
        return Mnemonic::NONE;
    }
}


// parse register name
static std::optional<OperandMemLoc> parse_Register(std::istream &input)
{
    auto initial_pos = input.tellg();

    char reg_name[AssemblyDef::MAX_REG_LEN + 1]{};
    unsigned int i = 0;

    char c;

    while (input && i < AssemblyDef::MAX_REG_LEN)
    {
        input >> c;

        if (std::iswspace(c) || c == AssemblyDef::instance.delimiter)
        {
            if (i)
                break;
            else
                continue;
        }

        reg_name[i++] = c;
    }

    if (!input && !(std::iswspace(c) || c == AssemblyDef::instance.delimiter))
    {
        return {};
    }

    auto found_it = AssemblyDef::instance.registers.find(std::string{reg_name});

    if (found_it == AssemblyDef::instance.registers.end())
    {
        input.seekg(initial_pos, std::ios::beg);
        return {};
    }

    return found_it->second;
}


// parse address
static std::optional<OperandMemLoc> parse_Address(std::istream &input)
{
    auto initial_pos = input.tellg();

    while (input)
    {
        char c;
        input >> c;

        if (!(std::iswspace(c) || c == AssemblyDef::instance.delimiter))
        {
            input.seekg(-1, std::ios::cur);
            break;
        }
    }

    return parse_Number<OperandMemLoc>(input);
}


// parse any memory location
static std::optional<OperandMemLoc> parse_MemoryLocation(std::istream &input, bool &is_address)
{
    auto mem_loc = parse_Register(input);
    if (mem_loc.has_value())
    {
        is_address = false;
        return mem_loc;
    }

    mem_loc = parse_Address(input);
    if (mem_loc.has_value())
    {
        is_address = true;
        return mem_loc;
    }

    return {};
}


// parse source operand
static SrcOperand parse_SrcOperand(std::istream &input, AssemblyParseState &parse_state)
{
    bool is_mem_loc_addr = false;
    auto mem_loc = parse_MemoryLocation(input, is_mem_loc_addr);
    if (mem_loc.has_value())
    {
        if (is_mem_loc_addr && mem_loc.value() < NUM_REGISTERS)
        {
            parse_state.messages.push_back("Address as a source operand can't be lower than number of registers.");
            return std::monostate{};
        }
        return mem_loc.value();
    }

    auto immediate = parse_Immediate(input);
    if (immediate.has_value())
    {
        return immediate.value();
    }


    std::string label = parse_Identifier(input);

    if (check_if_Reserved(label))
    {
        parse_state.messages.push_back("Invalid label - " + label);
        return {};
    }

    if (!label.empty())
    {
        return label;
    }

    return std::monostate{};
}


// parse destination operand
static DstOperand parse_DstOperand(std::istream &input, AssemblyParseState &parse_state, bool address_only)
{
    bool is_mem_loc_addr = false;
    auto mem_loc = parse_MemoryLocation(input, is_mem_loc_addr);
    if (mem_loc.has_value())
    {
        if (address_only && !is_mem_loc_addr)
        {
            parse_state.messages.push_back("Destination operand must be an address or a label in this context.");
            return std::monostate{};
        }
        if (!address_only && is_mem_loc_addr && mem_loc.value() < NUM_REGISTERS)
        {
            parse_state.messages.push_back("Address as destination operand can't be "
                                           "lower than number of registers in this context.");
            return std::monostate{};
        }

        return mem_loc.value();
    }

    std::string label = parse_Identifier(input);

    if (check_if_Reserved(label))
    {
        parse_state.messages.push_back("Invalid label - " + label);
        return {};
    }

    if (!label.empty())
    {
        return label;
    }

    return std::monostate{};
}


// check if mnemonic is a one of jumping mnemonics
static bool is_JMP(Mnemonic mnemonic)
{
    return
    mnemonic == Mnemonic::JE ||
    mnemonic == Mnemonic::JNE||
    mnemonic == Mnemonic::JLT||
    mnemonic == Mnemonic::JLE||
    mnemonic == Mnemonic::JGT||
    mnemonic == Mnemonic::JGE||
    mnemonic == Mnemonic::JMP;
}


// parse instruction
static std::optional<Instruction> parse_Instruction(std::istream &input, AssemblyParseState &parse_state)
{
    Mnemonic mnemonic = parse_Mnemonic(input);

    const char *src_expected = "Expected a source operand.";
    const char *dst_expected = "Expected a destination operand.";

    if (mnemonic == Mnemonic::NONE)
    {
        return {};
    }

    if (mnemonic == Mnemonic::NOP) // nop doesn't require any operands
    {
        return Instruction{.mnemonic = mnemonic};
    }

    if (mnemonic == Mnemonic::JMP) // jmp requires a single operand
    {
        DstOperand dst = parse_DstOperand(input, parse_state, true);
        if (is_in_Monostate(dst))
        {
            parse_state.messages.push_back(dst_expected);
            return {};
        }
        return Instruction{.mnemonic = mnemonic, .dst = dst};
    }

    SrcOperand src1 = parse_SrcOperand(input, parse_state);
    if (is_in_Monostate(src1))
    {
        parse_state.messages.push_back(src_expected);
        return {};
    }

    if (mnemonic == Mnemonic::MOV) // mov requires two operands
    {
        DstOperand dst = parse_DstOperand(input, parse_state, false);
        if (is_in_Monostate(dst))
        {
            parse_state.messages.push_back(dst_expected);
            return {};
        }
        return Instruction{.mnemonic = mnemonic, .src1 = src1, .dst = dst};
    }

    SrcOperand src2 = parse_SrcOperand(input, parse_state);
    if (is_in_Monostate(src2))
    {
        parse_state.messages.push_back(src_expected);
        return {};
    }

    // any other mnemonics require all three operands

    // jumping mnemonics require destination operand to be an address in memory or a label

    DstOperand dst = parse_DstOperand(input, parse_state, is_JMP(mnemonic));
    if (is_in_Monostate(dst))
    {
        parse_state.messages.push_back(dst_expected);
        return {};
    }

    return Instruction{mnemonic, src1, src2, dst};
}


// parse assembly
void parse_Assembly(std::istream &input, Assembly &assembly, std::vector<std::string> &messages)
{
    AssemblyParseState parse_state {
        .labels = assembly.labels,
        .messages = messages
    };

    // disable skipping whitespaces
    input.unsetf(std::ios::skipws);

    while (input)
    {
        if (parse_LabelDef(input, parse_state))
        {
            // check if the label is just a constant actually
            // constant's syntax is the same as immediate's one
            auto immediate = parse_Immediate(input);
            if (!immediate.has_value())
            {
                continue;
            }

            parse_state.last_defined_label_it->second = immediate.value();
            continue;
        }

        auto instruction = parse_Instruction(input, parse_state);
        if (instruction.has_value())
        {
            assembly.instructions.push_back(instruction.value());
            parse_state.next_instr_addr += instruction.value().size();
            continue;
        }

        input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}


// preprocess input aka remove comments
static void preprocess(std::istream &input, std::ostringstream &output)
{
    while (input)
    {
        std::string line;
        std::getline(input, line);
        size_t found_comment_pos = line.find(AssemblyDef::instance.comment);

        if (found_comment_pos != std::string::npos)
        {
            output << line.substr(0, found_comment_pos);
        }
        else
        {
            output << line;
        }

        output << std::endl;
    }
}


// convert mnemonic to opcode
static inline std::optional<char> assemble_Mnemonic(Mnemonic mnemonic, std::vector<std::string> &messages)
{
    auto found_opcode = AssemblyDef::instance.opcodes.find(mnemonic);
    if (found_opcode == AssemblyDef::instance.opcodes.end())
    {
        messages.push_back("Unknown instruction.");
        return {};
    }

    return found_opcode->second;
}


// convert memory location to binary representation
static inline std::optional<char> assemble_MemLoc(OperandMemLoc mem_loc, std::vector<std::string> &messages)
{
    if (mem_loc >= OPERAND_VALUE_LIMIT)
    {
        std::stringstream stream;
        stream << "Memory location can't be larger or equal than 0x" << std::hex << OPERAND_VALUE_LIMIT << '.';
        messages.push_back(stream.str());
        return {};
    }

    return mem_loc;
}


// convert immediate value to binary representation
static inline std::optional<char> assemble_Immediate(OperandImmediate immediate, std::vector<std::string> &messages)
{
    if (static_cast<uint32_t>(immediate) >= OPERAND_VALUE_LIMIT)
    {
        messages.push_back("Unsigned value of immediate can't be larger or equal than " + std::to_string(OPERAND_VALUE_LIMIT) + '.');
        return {};
    }

    return static_cast<char>(immediate);
}


// convert label to address or constant and convert it to binary representation
static inline std::optional<char> assemble_Label(
        const OperandStr &label, const Label2Int_Map &labels, std::vector<std::string> &messages
    )
{
    if (labels.find(label) == labels.end())
    {
        messages.push_back("Label " + label + " isn't defined.");
        return {};
    }

    int32_t label_val = labels.at(label);

    if (static_cast<uint32_t>(label_val) >= OPERAND_VALUE_LIMIT)
    {
        messages.push_back("Unsigned value of label can't be larger or equal than " + std::to_string(OPERAND_VALUE_LIMIT) + '.');
        return {};
    }

    return static_cast<char>(label_val);
}


// convert source operand to binary representation and tell if it was immediate
static std::optional<char> assemble_SrcOperand(
        const SrcOperand &src_operand,
        const Label2Int_Map &labels,
        bool &is_immediate,
        std::vector<std::string> &messages
    )
{
    if (auto *mem_loc = std::get_if<OperandMemLoc>(&src_operand))
    {
        is_immediate = false;
        return assemble_MemLoc(*mem_loc, messages);
    }
    else if (auto *immediate = std::get_if<OperandImmediate>(&src_operand))
    {
        is_immediate = true;
        return assemble_Immediate(*immediate, messages);
    }
    else if (auto *label = std::get_if<OperandStr>(&src_operand))
    {
        is_immediate = true;
        return assemble_Label(*label, labels, messages);
    }
    else
    {
        return 0; // if source operand is empty, 0 is the default binary for it
    }
}


// convert destination operand to binary representation
static std::optional<char> assemble_DstOperand(
        const DstOperand &dst_operand,
        const Label2Int_Map &labels,
        std::vector<std::string> &messages
    )
{
    if (auto *mem_loc = std::get_if<OperandMemLoc>(&dst_operand))
    {
        return assemble_MemLoc(*mem_loc, messages);
    }
    else if (auto *label = std::get_if<OperandStr>(&dst_operand))
    {
        return assemble_Label(*label, labels, messages);
    }
    else
    {
        return 0; // if destination operand is empty, 0 is the default binary for it
    }
}


// assemble parsed assembly
bool assemble(const Assembly &assembly, std::ostream &output, std::vector<std::string> &messages)
{
    bool keep_assembling = true;
    for (auto &&instr : assembly.instructions)
    {
        bool first_immediate = false, second_immediate = false;
        auto opcode   = assemble_Mnemonic(instr.mnemonic, messages);
        auto src1_val = assemble_SrcOperand(instr.src1, assembly.labels, first_immediate, messages);
        auto src2_val = assemble_SrcOperand(instr.src2, assembly.labels, second_immediate, messages);
        auto dst_val  = assemble_DstOperand(instr.dst,  assembly.labels, messages);

        // if an error happens, no further assembling keeps on but the assembly code will still continue being analyzed
        if (keep_assembling && !(opcode.has_value() && src1_val.has_value() && src2_val.has_value() && dst_val.has_value()))
            keep_assembling = false;

        if (keep_assembling)
        {
            if (first_immediate)
                opcode.value() |= AssemblyDef::FIRST_IMMEDIATE_BIT;
            if (second_immediate)
                opcode.value() |= AssemblyDef::SECOND_IMMEDIATE_BIT;

            char binary_instr[4] = {opcode.value(), src1_val.value(), src2_val.value(), dst_val.value()};
            output.write(binary_instr, 4);
        }
    }

    // success or fail
    return keep_assembling;
}


// final assemble function
bool assemble(std::istream &input, std::ostream &output, std::vector<std::string> &messages)
{
    // preprocess input
    std::ostringstream preprocessed_output;
    preprocess(input, preprocessed_output);

    // parse clean input
    std::istringstream clean_input(preprocessed_output.str());
    Assembly assembly;
    parse_Assembly(clean_input, assembly, messages);

    // assemble parsed assembly
    return assemble(assembly, output, messages);
}

