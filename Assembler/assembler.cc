#include "assembler.h"

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


static Op check_Op(const std::string &str)
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

static bool check_Identifier(const char *str, size_t num_chars)
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

static bool check_LabelDef(std::string &str)
{
    if (str.back() == ':')
    {
        return check_Identifier(str.c_str(), str.size() - 1);
    }
    else
    {
        return false;
    }
}


static unsigned int check_Register(const std::string &reg)
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

static unsigned int check_Address(const std::string &addr)
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

static std::optional<MemoryLocation> check_MemoryLocation(const std::string &mem_loc)
{
    unsigned int parsed_mem_loc = check_Register(mem_loc);
    if (parsed_mem_loc != NUM_REGISTERS)
    {
        return MemoryLocation{ static_cast<unsigned char>(parsed_mem_loc)};
    }

    parsed_mem_loc = check_Address(mem_loc);
    if (parsed_mem_loc != INSTRUCTION_OPERAND_SIZE)
    {
        return MemoryLocation{ static_cast<unsigned char>(parsed_mem_loc)};
    }

    return {};
}

static unsigned int check_Immediate(const std::string &immediate)
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

static std::optional<SrcType> check_SrcOperand(const std::string &token, AssemblyParseState &parse_state)
{
    auto mem_loc = check_MemoryLocation(token);
    if (mem_loc.has_value())
    {
        return mem_loc.value();
    }

    if (check_Identifier(token.c_str(), token.size()))
    {
        auto found_it = parse_state.label_to_index_map.find(token);
        if (found_it != parse_state.label_to_index_map.end())
        {
            return Label{found_it->second};
        }

        found_it = parse_state.const_to_index_map.find(token);
        if (found_it != parse_state.const_to_index_map.end())
        {
            return Constant{found_it->second};
        }
    }
    else
    {
        unsigned int immediate = check_Immediate(token);
        if (immediate != INSTRUCTION_OPERAND_SIZE)
        {
            return Immediate{immediate};
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
                                  "to form a complete instruction.";


using MaybeInstr = std::optional<Instruction>;


static MaybeInstr parse_Instruction_from_Op(Op op, AssemblyParseState &parse_state)
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

    auto src1 = check_SrcOperand(token, parse_state);
}



Assembly parse_Assembly(std::istream &code)
{
    Assembly assembly;

    while (code)
    {
        std::string word;
        code >> word;

        if (check_LabelDef(word))
        {

        }
    }

    return assembly;
}


void assemble(std::istream &code, std::ostream &output)
{
}

