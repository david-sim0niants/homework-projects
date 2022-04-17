#include "assembler.h"

#include <limits>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <optional>


struct AssemblyParseState
{
    uint32_t next_instr_addr = 0;
    Label2Int_Map &labels;
    Label2Int_Map::iterator last_defined_label_it = {};
    std::vector<std::string> &messages;
};


class AssemblyDef
{
public:
    char immediate_sign = '#';
    char delimiter = ',';
    const char *comment = "//";
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
    std::unordered_map<std::string, size_t> registers;

    static constexpr unsigned char CONDITIONAL_BIT = 0x20;
    static constexpr unsigned char FIRST_IMMEDIATE_BIT = 0x40;
    static constexpr unsigned char SECOND_IMMEDIATE_BIT = 0x80;

    std::unordered_map<Mnemonic, unsigned char> opcodes = {
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

    static const AssemblyDef instance;
    static constexpr unsigned int MAX_MNM_LEN = 3;
    static constexpr unsigned int MAX_REG_LEN = 3;

private:
    AssemblyDef()
    {
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


inline const AssemblyDef AssemblyDef::instance;


template<typename... Types>
static bool is_in_Monostate(std::variant<Types...> variant)
{
    return std::holds_alternative<std::monostate>(variant);
}


static std::optional<OperandImmediate> parse_Immediate(std::istream &input)
{
    auto initial_pos = input.tellg();

    OperandImmediate immediate;

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
            input.seekg(initial_pos, std::ios::beg);
            return {};
        }

        break;
    }

    if (!input)
    {
        input.seekg(initial_pos, std::ios::beg);
        return {};
    }

    input >> std::dec >> immediate;

    if (input.fail())
    {
        input.clear();
        input.seekg(initial_pos, std::ios::beg);
        return {};
    }

    if (input)
    {
        char c;
        input >> c;

        if (!std::iswspace(c) && c != AssemblyDef::instance.delimiter)
        {
            input.seekg(initial_pos, std::ios::beg);
            return {};
        }
    }

    return immediate;
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

            if ((c < 'A' || c > 'Z') && (c < 'a' || c > 'z') && c != '.' && c != '_')
            {
                break;
            }

            identifier.push_back(c);
        }
        else
        {
            if (std::iswspace(c) ||
                (c < 'A' || c > 'Z') && (c < 'a' || c > 'z') &&
                (c < '0' || c > '9') && c != '.' && c != '_')
            {
                break;
            }

            identifier.push_back(c);
        }

    }

    if (identifier.empty())
    {
        input.seekg(-chars_read - 1, std::ios::cur);
    }
    else
    {
        input.seekg(-1, std::ios::cur);
    }

    return identifier;
}


static bool check_if_Reserved(std::string &identifier)
{
    return !
    (AssemblyDef::instance.mnemonics.find(identifier) == AssemblyDef::instance.mnemonics.end()
    &&
    AssemblyDef::instance.registers.find(identifier) == AssemblyDef::instance.registers.end());
}


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


static Mnemonic parse_Mnemonic(std::istream &input)
{
    auto initial_pos = input.tellg();

    std::string token;
    input.setf(std::ios::skipws);
    input >> token;
    input.unsetf(std::ios::skipws);

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


static std::optional<OperandMemLoc> parse_Address(std::istream &input, AssemblyParseState &parse_state)
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

    OperandMemLoc addr;
    input >> std::hex >> addr;

    if (input.fail())
    {
        input.clear();
        input.seekg(initial_pos, std::ios::beg);
        return {};
    }

    if (input)
    {
        char c;
        input >> c;

        if (!(std::iswspace(c) || c == AssemblyDef::instance.delimiter))
        {
            input.seekg(initial_pos, std::ios::beg);
            return {};
        }
    }

    if (addr < NUM_REGISTERS)
    {
        parse_state.messages.push_back("Address lower than number of registers.");
        input.seekg(initial_pos, std::ios::beg);
        return {};
    }

    return addr;
}


static std::optional<OperandMemLoc> parse_MemoryLocation(std::istream &input, AssemblyParseState &parse_state)
{
    auto mem_loc = parse_Register(input);
    if (mem_loc.has_value())
    {
        return mem_loc;
    }

    mem_loc = parse_Address(input, parse_state);
    if (mem_loc.has_value())
    {
        return mem_loc;
    }

    return {};
}


static SrcOperand parse_SrcOperand(std::istream &input, AssemblyParseState &parse_state)
{
    auto mem_loc = parse_MemoryLocation(input, parse_state);
    if (mem_loc.has_value())
    {
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


static DstOperand parse_DstOperand(std::istream &input, AssemblyParseState &parse_state)
{
    auto mem_loc = parse_MemoryLocation(input, parse_state);
    if (mem_loc.has_value())
    {
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


static std::optional<Instruction> parse_Instruction(std::istream &input, AssemblyParseState &parse_state)
{
    Mnemonic mnemonic = parse_Mnemonic(input);

    const char *src_expected = "Expected a source operand.";
    const char *dst_expected = "Expected a destination operand.";

    if (mnemonic == Mnemonic::NONE)
    {
        return {};
    }

    if (mnemonic == Mnemonic::NOP)
    {
        return Instruction{.mnemonic = mnemonic};
    }

    if (mnemonic == Mnemonic::JMP)
    {
        DstOperand dst = parse_DstOperand(input, parse_state);
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

    if (mnemonic == Mnemonic::MOV)
    {
        DstOperand dst = parse_DstOperand(input, parse_state);
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

    DstOperand dst = parse_DstOperand(input, parse_state);
    if (is_in_Monostate(dst))
    {
        parse_state.messages.push_back(dst_expected);
        return {};
    }

    return Instruction{mnemonic, src1, src2, dst};
}


void parse_Assembly(std::istream &input, Assembly &assembly, std::vector<std::string> &messages)
{
    AssemblyParseState parse_state {
        .labels = assembly.labels,
        .messages = messages
    };

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


static inline std::optional<unsigned char> assemble_Mnemonic(Mnemonic mnemonic, std::vector<std::string> &messages)
{
    auto found_opcode = AssemblyDef::instance.opcodes.find(mnemonic);
    if (found_opcode == AssemblyDef::instance.opcodes.end())
    {
        messages.push_back("Unknown instruction.");
        return {};
    }

    return found_opcode->second;
}


static inline std::optional<unsigned char> assemble_MemLoc(OperandMemLoc mem_loc, std::vector<std::string> &messages)
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


static inline std::optional<unsigned char> assemble_Immediate(OperandImmediate immediate, std::vector<std::string> &messages)
{
    if (static_cast<std::make_unsigned<decltype(immediate)>::type>(immediate) >= OPERAND_VALUE_LIMIT)
    {
        messages.push_back("Unsigned value of immediate can't be larger or equal than " + std::to_string(OPERAND_VALUE_LIMIT) + '.');
        return {};
    }

    return immediate;
}


static inline std::optional<unsigned char> assemble_Label(
        const OperandStr &label, const Label2Int_Map &labels, std::vector<std::string> &messages
    )
{
    if (labels.find(label) == labels.end())
    {
        messages.push_back("Label " + label + " isn't defined.");
        return {};
    }

    int32_t label_val = labels.at(label);

    if (static_cast<std::make_unsigned<decltype(label_val)>::type>(label_val) >= OPERAND_VALUE_LIMIT)
    {
        messages.push_back("Unsigned value of label can't be larger or equal than " + std::to_string(OPERAND_VALUE_LIMIT) + '.');
        return {};
    }

    return label_val;
}


static std::optional<unsigned char> assemble_SrcOperand(
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
        is_immediate = false;
        return assemble_Label(*label, labels, messages);
    }
    else
    {
        return 0;
    }
}


static std::optional<unsigned char> assemble_DstOperand(
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
        return 0;
    }
}


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

        if (keep_assembling && !(opcode.has_value() && src1_val.has_value() && src2_val.has_value() && dst_val.has_value()))
            keep_assembling = false;

        if (keep_assembling)
        {
            if (first_immediate)
                opcode.value() |= AssemblyDef::FIRST_IMMEDIATE_BIT;
            if (second_immediate)
                opcode.value() |= AssemblyDef::SECOND_IMMEDIATE_BIT;

            unsigned char binary_instr[4] = {opcode.value(), src1_val.value(), src2_val.value(), dst_val.value()};
            output.write(reinterpret_cast<char *>(binary_instr), 4);
        }
    }

    if (!keep_assembling)
    {
        output.fail();
    }

    return keep_assembling;
}


bool assemble(std::istream &input, std::ostream &output, std::vector<std::string> &messages)
{
    std::ostringstream preprocessed_output;
    preprocess(input, preprocessed_output);

    std::istringstream clean_input(preprocessed_output.str());
    Assembly assembly;
    parse_Assembly(clean_input, assembly, messages);

    return assemble(assembly, output, messages);
}

