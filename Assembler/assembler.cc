#include "assembler.h"

#include <istream>
#include <iterator>
#include <sstream>
#include <limits>
#include <string>
#include <unordered_map>
#include <optional>
#include <variant>
#include <algorithm>



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
    else if (str == "JMP" || str == "jmp")
        return Op::JMP;
    else if (str == "MOV" || str == "mov")
        return Op::MOV;
    else
        return Op::NONE;
}

static bool can_parse_Identifier(const char *str, size_t num_chars)
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
        return can_parse_Identifier(str.c_str(), str.size() - 1);
    }
    else
    {
        return false;
    }
}


static unsigned int parse_Register(const std::string &reg)
{
    if (reg.size() >= 4 || reg.size() < 2 || reg.front() != 'r' && reg.front() != 'R')
    {
        return NUM_REGISTERS;
    }

    if (reg[1] < '0' || reg[1] > '9')
    {
        return NUM_REGISTERS;
    }

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

static unsigned int parse_Address(const std::string &addr)
{
    if ((addr[0] != '0' || addr[1] != 'x') &&
        (addr.front() < '0' || addr.front() > 'f' ||
         addr.size() > 2 || addr.empty()))
    {
        return INSTRUCTION_OPERAND_MAX_VALUE;
    }

    size_t next_char_i = 0;
    int parsed_addr = std::stoi(addr, &next_char_i, 16);

    if (next_char_i >= addr.size() && parsed_addr < INSTRUCTION_OPERAND_MAX_VALUE)
    {
        return parsed_addr;
    }
    else
    {
        return INSTRUCTION_OPERAND_MAX_VALUE;
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
    if (parsed_mem_loc != INSTRUCTION_OPERAND_MAX_VALUE)
    {
        return MemoryLocation{ static_cast<unsigned char>(parsed_mem_loc)};
    }

    return {};
}

static std::optional<unsigned int> parse_Immediate(const std::string &immediate)
{
    if (immediate.front() != '$' ||
        immediate.size() > 12 ||
        immediate.size() < 2  ||
        ((immediate[1] > '9' || immediate[1] < '0') && immediate[1] != '-') ||
        ((immediate[2] > '9' || immediate[2] < '0') && immediate[1] == '-'))
    {
        return {};
    }

    size_t next_char_i = 0;
    int parsed_imm = std::stoi(immediate.substr(1, immediate.size() - 1), &next_char_i);

    if (next_char_i >= immediate.size() - 1)
    {
        return parsed_imm;
    }
    else
    {
        return {};
    }
}

static SrcType parse_SrcOperand(const std::string &token, AssemblyParseState &parse_state)
{
    // try parse memory location
    auto mem_loc = parse_MemoryLocation(token);
    if (mem_loc.has_value())
    {
        return mem_loc.value();
    }

    // if failed to parse an identifier, try parse an immediate value
    auto immediate = parse_Immediate(token);
    if (immediate.has_value())
    {
        return Immediate{immediate.value()};
    }
    else if (can_parse_Identifier(token.c_str(), token.size())) // try parse identifier
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
    return std::monostate{};
}

static DstType parse_DstOperand(const std::string &token, AssemblyParseState &parse_state)
{
    auto &code = parse_state.code;

    // try parse memory location
    auto mem_loc = parse_MemoryLocation(token);
    if (mem_loc.has_value())
    {
        return mem_loc.value();
    }

    // try parse identifier
    if (can_parse_Identifier(token.c_str(), token.size()))
    {
        // check for label
        auto found_it = parse_state.label_to_index_map.find(token);
        if (found_it != parse_state.label_to_index_map.end())
        {
            return Label{found_it->second};
        }
    }

    return std::monostate{};
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


static void trim_Commas(std::string &str)
{
    auto no_commas = [](unsigned char ch) { return ch != ','; };
    str.erase(str.begin(), std::find_if(str.begin(),  str.end(),  no_commas));
    str.erase(std::find_if(str.rbegin(), str.rend(), no_commas).base(), str.end());
}


static const std::string
label_redefinition_error = "Error: redefinition of the same label",
missing_operands_in_instr_error = "Error: some operands are missing"
                                  " to form a complete instruction.",
expected_opname_error = "Error: expected an operation.",
missing_const_name_after_constdef = "Error: missing const name after constdef.",
invalid_const_name = "Error: Invalid const name.",
identifier_already_declared = "Error: identifier is already declared.",
missing_const_value = "Error: missing const value.",
const_value_must_be_immediate = "Error: const value must be an immediate.";


template<typename... Types>
bool is_in_Monostate(std::variant<Types...> variant)
{
    return std::holds_alternative<std::monostate>(variant);
}


static std::optional<Instruction> parse_Instruction_from_Op(Op op, AssemblyParseState &parse_state)
{
    auto &code = parse_state.code;
    auto log_error = [&parse_state](const std::string &token = "") {
        parse_state.messages.push_back(missing_operands_in_instr_error);
        parse_state.messages.push_back("Error: unknown source operand - " + token + '.');
    };

    if (!code)
    {
        log_error();
        return {};
    }

    OperandsOrder operands_order = get_OperandsOrder_from_Op(op);
    std::string token;

    if (!code)
    {
        log_error();
        return {};
    }

    code >> token;
    trim_Commas(token);

    SrcType src1 = std::monostate{};
    if (operands_order == OperandsOrder::SRC_DST || operands_order == OperandsOrder::SRC1_SRC2_DST)
    {
        src1 = parse_SrcOperand(token, parse_state);
        if (is_in_Monostate(src1) || !code)
        {
            log_error(token);
            return {};
        }

        code >> token;
        trim_Commas(token);
    }


    SrcType src2 = std::monostate{};
    if (operands_order == OperandsOrder::SRC1_SRC2_DST)
    {
        src2 = parse_SrcOperand(token, parse_state);

        if (is_in_Monostate(src2) || !code)
        {
            log_error(token);
            return {};
        }

        code >> token;
        trim_Commas(token);
    }


    DstType dst = std::monostate{};
    if (operands_order == OperandsOrder::DST ||
        operands_order == OperandsOrder::SRC_DST ||
        operands_order == OperandsOrder::SRC1_SRC2_DST)
    {
        dst = parse_DstOperand(token, parse_state);

        if (is_in_Monostate(dst))
        {
            log_error(token);
            return {};
        }
    }

    return Instruction{NO_LABEL, op, src1, src2, dst};
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

bool parse_ConstDef(AssemblyParseState &parse_state)
{
    std::string token;

    if (!parse_state.code)
    {
        parse_state.messages.push_back(missing_const_name_after_constdef);
        return false;
    }

    parse_state.code >> token;

    if (!can_parse_Identifier(token.c_str(), token.size()))
    {
        parse_state.messages.push_back(invalid_const_name);
        return false;
    }

    auto const_found_it = parse_state.const_to_index_map.find(token);
    auto label_found_it = parse_state.label_to_index_map.find(token);

    if (const_found_it == parse_state.const_to_index_map.end() &&
        label_found_it == parse_state.label_to_index_map.end())
    {
        parse_state.messages.push_back(identifier_already_declared);
        return false;
    }

    if (!parse_state.code)
    {
        parse_state.messages.push_back(missing_const_value);
        return false;
    }

    parse_state.code >> token;

    auto immediate = parse_Immediate(token);
    if (!immediate.has_value())
    {
        parse_state.messages.push_back(const_value_must_be_immediate);
        return false;
    }

    return true;
}



Assembly parse_Assembly(std::istream &code, std::vector<std::string> &messages)
{
    StringToIndexMap label_to_index_map, const_to_index_map;
    label_to_index_map["<no_label>"] = NO_LABEL;

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

        if (token.find("//") != std::string::npos)
        {
            code.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }


        if (token == "constdef")
        {
            parse_ConstDef(parse_state);
            continue;
        }


        unsigned int label_index = NO_LABEL;

        std::optional<Instruction> maybe_instr;
        if (can_parse_LabelDef(token))
        {
            // check for label
            std::string label = token.substr(0, token.size() - 1);
            auto found_label_it = label_to_index_map.find(label);
            auto found_const_it = const_to_index_map.find(label);

            if (found_label_it == label_to_index_map.end() && found_const_it == const_to_index_map.end())
            {
                label_index = label_to_index_map.size();
                label_to_index_map[label] = label_index;
            }
            else
            {
                messages.push_back(identifier_already_declared);
            }

            maybe_instr = parse_Instruction_from_Label(label_index, parse_state);
        }
        else
        {
            Op op = parse_Op(token);
            if (op != Op::NONE)
            {
                maybe_instr = parse_Instruction_from_Op(op, parse_state);
            }
        }

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

