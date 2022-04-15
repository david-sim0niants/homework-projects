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
#include <cstring>



using StringToUintMap = std::unordered_map<std::string, unsigned int>;


struct AssemblyParseState
{
    std::istream &code;
    StringToUintMap &label_to_index_map;
    StringToUintMap &const_to_value_map;
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

static std::optional<unsigned char> parse_MemoryLocation(const std::string &mem_loc)
{
    unsigned int parsed_mem_loc = parse_Register(mem_loc);
    if (parsed_mem_loc != NUM_REGISTERS)
    {
        return { static_cast<unsigned char>(parsed_mem_loc)};
    }

    parsed_mem_loc = parse_Address(mem_loc);
    if (parsed_mem_loc != INSTRUCTION_OPERAND_MAX_VALUE)
    {
        return { static_cast<unsigned char>(parsed_mem_loc)};
    }

    return {};
}

static std::optional<unsigned char> parse_Immediate(const std::string &immediate)
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

static std::optional<unsigned char> parse_SrcOperand(const std::string &token, AssemblyParseState &parse_state, bool &is_label)
{
    is_label = false;
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
        return immediate.value();
    }
    else if (can_parse_Identifier(token.c_str(), token.size())) // try parse identifier
    {
        // check for label
        auto found_it = parse_state.label_to_index_map.find(token);
        if (found_it != parse_state.label_to_index_map.end())
        {
            is_label = true;
            return found_it->second;
        }

        // check for const
        found_it = parse_state.const_to_value_map.find(token);
        if (found_it != parse_state.const_to_value_map.end())
        {
            return found_it->second;
        }
    }

    // fail if no good case returned
    return {};
}

static std::optional<unsigned char> parse_DstOperand(const std::string &token, AssemblyParseState &parse_state, bool &is_label)
{
    auto &code = parse_state.code;
    is_label = false;

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
            is_label = true;
            return found_it->second;
        }
    }

    return {};
}


static void trim_Commas(std::string &str)
{
    auto no_commas = [](unsigned char ch) { return ch != ','; };
    str.erase(str.begin(), std::find_if(str.begin(),  str.end(),  no_commas));
    str.erase(std::find_if(str.rbegin(), str.rend(), no_commas).base(), str.end());
}


static const std::string
label_redefinition_error = "Error: redefinition of the same label",
label_not_defined_error = "Error: label is not defined.",
missing_operands_in_instr_error = "Error: some operands are missing"
                                  " to form a complete instruction.",
expected_opname_error = "Error: expected an operation.",
missing_const_name_after_constdef = "Error: missing const name after constdef.",
invalid_const_name = "Error: Invalid const name.",
identifier_already_declared = "Error: identifier is already declared.",
missing_const_value = "Error: missing const value.",
const_value_must_be_immediate = "Error: const value must be an immediate.";


template<typename... Types>
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

    std::optional<unsigned char> src1;
    bool is_src1_label = false;
    if (operands_order == OperandsOrder::SRC_DST || operands_order == OperandsOrder::SRC1_SRC2_DST)
    {
        src1 = parse_SrcOperand(token, parse_state, is_src1_label);
        if (!src1.has_value() || !code)
        {
            log_error(token);
            return {};
        }

        code >> token;
        trim_Commas(token);
    }


    std::optional<unsigned char> src2 = {};
    bool is_src2_label = false;
    if (operands_order == OperandsOrder::SRC1_SRC2_DST)
    {
        src2 = parse_SrcOperand(token, parse_state, is_src2_label);

        if (!src2.has_value() || !code)
        {
            log_error(token);
            return {};
        }

        code >> token;
        trim_Commas(token);
    }


    std::optional<unsigned char> dst;
    bool is_dst_label = false;
    if (operands_order == OperandsOrder::DST ||
        operands_order == OperandsOrder::SRC_DST ||
        operands_order == OperandsOrder::SRC1_SRC2_DST)
    {
        dst = parse_DstOperand(token, parse_state, is_dst_label);

        if (!dst.has_value())
        {
            log_error(token);
            return {};
        }
    }

    return Instruction{NO_LABEL, op, src1, src2, dst, is_src1_label, is_src2_label, is_dst_label};
}


static std::optional<Instruction>
parse_Instruction(unsigned char label_index, AssemblyParseState &parse_state)
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

    auto instr = parse_Instruction_from_Op(op, parse_state);

    if (instr.has_value())
    {
        instr.value().label_index = label_index;
    }

    return instr;
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

    auto const_found_it = parse_state.const_to_value_map.find(token);
    auto label_found_it = parse_state.label_to_index_map.find(token);

    if (const_found_it == parse_state.const_to_value_map.end() &&
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

    parse_state.const_to_value_map[token] = immediate.value();

    return true;
}


static void parse_AllDefs(std::istream &code, AssemblyParseState &parse_state)
{
    auto &label_to_index_map = parse_state.label_to_index_map;
    auto &const_to_value_map = parse_state.const_to_value_map;

    while (code)
    {
        std::string token;
        code >> token;

        if (!can_parse_LabelDef(token))
        {
            if (token == "constdef")
            {
                parse_ConstDef(parse_state);
            }
            continue;
        }

        auto label = token.substr(0, token.size() - 1);

        if (label_to_index_map.find(label) != label_to_index_map.end())
        {
            parse_state.messages.push_back(label_redefinition_error);
            continue;
        }

        if (const_to_value_map.find(label) != const_to_value_map.end())
        {
            parse_state.messages.push_back(identifier_already_declared);
        }

        label_to_index_map[label] = label_to_index_map.size();
    }

    code.clear();
    code.seekg(0, std::ios::beg);
}


Assembly parse_Assembly(std::istream &code, std::vector<std::string> &messages)
{
    StringToUintMap label_to_index_map, const_to_value_map;

    AssemblyParseState parse_state {
        .code = code,
        .label_to_index_map = label_to_index_map,
        .const_to_value_map = const_to_value_map,
        .messages = messages
    };


    parse_AllDefs(code, parse_state);


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
            continue;
        }


        std::optional<Instruction> maybe_instr;
        if (can_parse_LabelDef(token))
        {
            // check for label
            std::string label = token.substr(0, token.size() - 1);
            auto found_label_it = label_to_index_map.find(label);

            if (found_label_it == label_to_index_map.end())
                messages.push_back(label_not_defined_error);
            
            maybe_instr = parse_Instruction(found_label_it->second, parse_state);
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


static const unsigned int FIRST_IMMEDIATE_BIT = 64, SECOND_IMMEDIATE_BIT = 128;
static const unsigned int JMP_BIT = 32;

static const std::unordered_map<Op, unsigned char> op_to_opcode_map = {
    {Op::ADD, 0},
    {Op::SUB, 1},
    {Op::AND, 2},
    {Op::OR,  3},
    {Op::NOT, 4},
    {Op::XOR, 5},
    {Op::MUL, 6},
    {Op::JE,  0 | JMP_BIT},
    {Op::JNE, 1 | JMP_BIT},
    {Op::JLT, 2 | JMP_BIT},
    {Op::JLE, 3 | JMP_BIT},
    {Op::JGT, 4 | JMP_BIT},
    {Op::JGE, 5 | JMP_BIT},
    {Op::JMP, 0 | FIRST_IMMEDIATE_BIT | SECOND_IMMEDIATE_BIT},
    {Op::MOV, 3 | SECOND_IMMEDIATE_BIT}
};


void assemble(std::istream &code, std::ostream &output, std::vector<std::string> &messages)
{
    Assembly assembly = parse_Assembly(code, messages);

    auto instructions = assembly.get_Instructions();

    std::unordered_map<unsigned char, unsigned char> label_index_to_addr_map;

    unsigned char instr_addr = 0;
    for (auto &&instr : instructions)
    {
        label_index_to_addr_map[instr.label_index] = instr_addr;
        instr_addr += 4;
    }

    for (auto &&instr : instructions)
    {
        if (instr.op == Op::NONE)
        {
            continue;
        }

        unsigned char instr_bin[4] {};
        instr_bin[0] = op_to_opcode_map.at(instr.op);

        instr_bin[1] = instr.src1_label ? label_index_to_addr_map[instr.src1_label] : instr.src1.value();
        if (instr.op != Op::MOV && instr.op != Op::JMP)
        {
            instr_bin[2] = instr.src2_label ? label_index_to_addr_map[instr.src2_label] : instr.src2.value();
        }

        if (instr.op == Op::JMP)
        {
            instr_bin[3] = NUM_REGISTERS - 1;
        }
        else
        {
            instr_bin[3] = instr.dst_label ? label_index_to_addr_map[instr.dst_label] : instr.dst.value();
        }


        output.write(reinterpret_cast<char *>(instr_bin), 1);
    }
}

