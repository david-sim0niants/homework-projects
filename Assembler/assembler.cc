#include "assembler.h"

#include <istream>
#include <sstream>
#include <limits>
#include <string>
#include <unordered_map>
#include <optional>



using StringToIndexMap = std::unordered_map<std::string, unsigned int>;

struct AssemblyParseState
{
    std::istream &code;
    StringToIndexMap &label_to_index_map;
    StringToIndexMap &const_to_index_map;
    std::vector<std::string> &messages;
};


static Op parse_Op(const std::string &str)
{
    if (str == "add" || str == "ADD")
        return Op::ADD;
    else if (str == "SUB" || str == "sub")
        return Op::SUB;
    else if (str == "OR"  || str ==  "or")
        return Op::OR;
    else if (str == "NOT" || str == "not")
        return Op::NOT;
    else if (str == "AND" || str == "and")
        return Op::AND;
    else if (str == "XOR" || str == "xor")
        return Op::XOR;
    else if (str == "MUL" || str == "mul")
        return Op::MUL;
    else if (str == "JE"  || str ==  "je")
        return Op::JE;
    else if (str == "JNE" || str == "jne")
        return Op::JNE;
    else if (str == "JLT" || str == "jlt")
        return Op::JLT;
    else if (str == "JLE" || str == "jle")
        return Op::JLE;
    else if (str == "JGT" || str == "jgt")
        return Op::JGT;
    else if (str == "JGE" || str == "jge")
        return Op::JGE;
    else
        return Op::NONE;
}

static bool parse_Identifier(const char *str, size_t num_chars)
{
    if (!num_chars)
        return false;

    char c = str[0];
    if ((c < 'A' || c > 'Z') && (c < 'a' || c > 'z') && c != '.' && c != '_')
    {
        return false;
    }

    for (size_t i = 1; i < num_chars; ++i)
    {
        c = str[i];
        if ((c < 'A' || c > 'Z') && (c < 'a' || c > 'z') && (c < '0' || c > '9') && c != '.' && c != '_')
        {
            return false;
        }
    }

    return true;
}

static bool can_parse_LabelDef(std::string &str)
{
    if (str.back() == ':')
    {
        return parse_Identifier(str.c_str(), str.size() - 1);
    }
    else
    {
        return false;
    }
}


static unsigned int parse_Register(const std::string &reg)
{
    if (reg.front() == 'r' || reg.front() == 'R')
    {
        size_t next_char_i;
        int reg_i = std::stoi(reg.substr(1, reg.size() - 1), &next_char_i);

        if (next_char_i < reg.size() - 1 || reg_i < 0 || reg_i >= NUM_REGISTERS)
        {
            return NUM_REGISTERS; // return number of registers in case of fail
        }
        else
        {
            return reg_i;
        }
    }
    else
    {
        return NUM_REGISTERS;
    }
}

static unsigned int parse_Address(const std::string &addr)
{
    size_t next_char_i = 0;
    int parsed_addr = std::stoi(addr, &next_char_i);

    if (next_char_i >= addr.size() && NUM_REGISTERS <= parsed_addr && parsed_addr < INSTRUCTION_OPERAND_SIZE)
    {
        return parsed_addr;
    }
    else
    {
        return INSTRUCTION_OPERAND_SIZE;
    }
}

static std::optional<MemoryLocation> parse_MemoryLocation(const std::string &mem_loc)
{
    unsigned int parsed_mem_loc = parse_Register(mem_loc);
    if (parsed_mem_loc != NUM_REGISTERS)
    {
        return MemoryLocation{ static_cast<unsigned char>(parsed_mem_loc)};
    }

    parsed_mem_loc = parse_Address(mem_loc);
    if (parsed_mem_loc != INSTRUCTION_OPERAND_SIZE)
    {
        return MemoryLocation{ static_cast<unsigned char>(parsed_mem_loc)};
    }

    return {};
}

static unsigned int parse_Immediate(const std::string &immediate)
{
    if (immediate.front() == '$')
    {
        size_t next_char_i = 0;
        int parsed_imm = std::stoi(immediate.substr(1, immediate.size() - 1), &next_char_i);

        if (next_char_i >= immediate.size() - 1)
        {
            return parsed_imm;
        }
    }
    return INSTRUCTION_OPERAND_SIZE;
}

static std::optional<SrcType> parse_SrcOperand(const std::string &token, AssemblyParseState &parse_state)
{
    // try parse memory location
    auto mem_loc = parse_MemoryLocation(token);
    if (mem_loc.has_value())
    {
        return mem_loc.value();
    }

    // if failed to parse an identifier, try parse an immediate value
    unsigned int immediate = parse_Immediate(token);
    if (immediate != INSTRUCTION_OPERAND_SIZE)
    {
        return Immediate{immediate};
    }
    else if (parse_Identifier(token.c_str(), token.size())) // try parse identifier
    {
        // check for label
        auto found_it = parse_state.label_to_index_map.find(token);
        if (found_it != parse_state.label_to_index_map.end())
        {
            return Label{found_it->second};
        }

        // check for const
        found_it = parse_state.const_to_index_map.find(token);
        if (found_it != parse_state.const_to_index_map.end())
        {
            return Constant{found_it->second};
        }
    }

    // fail if no good case returned
    return {};
}

static std::optional<DstType> parse_DstOperand(const std::string &token, AssemblyParseState &parse_state)
{
    auto &code = parse_state.code;

    // try parse memory location
    auto mem_loc = parse_MemoryLocation(token);
    if (mem_loc.has_value())
    {
        return mem_loc.value();
    }

    // try parse identifier
    if (parse_Identifier(token.c_str(), token.size()))
    {
        // check for label
        auto found_it = parse_state.label_to_index_map.find(token);
        if (found_it != parse_state.label_to_index_map.end())
        {
            return Label{found_it->second};
        }
    }

    return {};
}


static bool assign_String(const std::string &str, StringToIndexMap &string_to_index_map, size_t &index)
{
    auto found_it = string_to_index_map.find(str);
    if (found_it == string_to_index_map.end())
    {
        index = string_to_index_map.size();
        string_to_index_map[str] = index;
        return true;
    }
    return false;
}


static const char
*label_redefinition_error = "Error: redefinition of the same label",
*missing_operands_in_instr_error = "Error: some operands are missing"
                                  "to form a complete instruction.",
*expected_opname_error = "Error: expected an operation.";


static std::optional<Instruction> parse_Instruction_from_Op(Op op, AssemblyParseState &parse_state)
{
    auto &code = parse_state.code;
    auto log_error = [&parse_state]() {
        parse_state.messages.push_back(missing_operands_in_instr_error);
    };

    if (!code)
    {
        log_error();
        return {};
    }

    std::string token;
    code >> token;

    auto src1 = parse_SrcOperand(token, parse_state);

    if (!src1.has_value() || !code)
    {
        log_error();
        return {};
    }

    code >> token;

    auto src2 = parse_SrcOperand(token, parse_state);

    if (!src2.has_value() || !code)
    {
        log_error();
        return {};
    }

    code >> token;

    auto dst = parse_DstOperand(token, parse_state);

    if (!dst.has_value())
    {
        log_error();
        return {};
    }

    return Instruction{NO_LABEL, op, src1.value(), src2.value(), dst.value()};
}


static std::optional<Instruction>
parse_Instruction_from_Label(unsigned int label_index, AssemblyParseState &parse_state)
{
    if (!parse_state.code)
    {
        parse_state.messages.push_back(expected_opname_error);
        return {};
    }

    std::string op_tok;
    parse_state.code >> op_tok;

    Op op = parse_Op(op_tok);
    if (op == Op::NONE)
    {
        parse_state.messages.push_back(expected_opname_error);
        return {};
    }

    auto maybe_instr = parse_Instruction_from_Op(op, parse_state);
    if (maybe_instr.has_value())
    {
        maybe_instr.value().label_index = label_index;
    }

    return maybe_instr;
}



Assembly parse_Assembly(std::istream &code, std::vector<std::string> &messages)
{
    StringToIndexMap label_to_index_map, const_to_index_map;
    label_to_index_map[":"] = NO_LABEL;

    AssemblyParseState parse_state {
        .code = code,
        .label_to_index_map = label_to_index_map,
        .const_to_index_map = const_to_index_map,
        .messages = messages
    };


    Assembly assembly;

    while (code)
    {
        std::string token;
        code >> token;

        unsigned int label_index = NO_LABEL;

        if (can_parse_LabelDef(token))
        {
            // check for label
            auto found_it = label_to_index_map.find(token);
            if (found_it == label_to_index_map.end())
            {
                label_index = label_to_index_map.size();
                label_to_index_map[token.substr(0, token.size() - 1)] = label_index;
            }
        }

        auto maybe_instr = parse_Instruction_from_Label(label_index, parse_state);
        if (maybe_instr.has_value())
        {
            assembly.add_Instruction(maybe_instr.value());
        }
    }


    return assembly;
}


void assemble(std::istream &code, std::ostream &output, std::vector<std::string> &messages)
{
    Assembly assembly = parse_Assembly(code, messages);
}



void Assembly::add_Instruction(const Instruction &instruction)
{
    instructions.push_back(instruction);
}

