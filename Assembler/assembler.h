#ifndef __ASSEMBLER_H__
#define __ASSEMBLER_H__

#include <istream>
#include <ostream>
#include <optional>
#include <vector>



enum class Op
{
    NONE, ADD, SUB, OR, NOT, AND, XOR, MUL, JE, JNE, JLT, JLE, JGT, JGE, JMP, MOV
};


enum class OperandsOrder
{
    NONE, DST, SRC_DST, SRC1_SRC2_DST
};


inline OperandsOrder get_OperandsOrder_from_Op(Op op)
{
    switch (op)
    {
    case Op::NONE:
        return OperandsOrder::NONE;
    case Op::ADD:
    case Op::SUB:
    case Op::OR:
    case Op::AND:
    case Op::XOR:
    case Op::MUL:
    case Op::JE:
    case Op::JNE:
    case Op::JLT:
    case Op::JLE:
    case Op::JGT:
    case Op::JGE:
        return OperandsOrder::SRC1_SRC2_DST;
    case Op::NOT:
    case Op::MOV:
        return OperandsOrder::SRC_DST;
    case Op::JMP:
        return OperandsOrder::DST;
    default:
        return OperandsOrder::NONE;
    }
}


constexpr unsigned int NUM_REGISTERS = 16;
constexpr unsigned int NUM_GP_REGISTERS = NUM_REGISTERS - 2;
constexpr unsigned int IO_REG_INDEX = NUM_GP_REGISTERS;
constexpr unsigned int COUNTER_INDEX = NUM_GP_REGISTERS + 1;
constexpr unsigned int INSTRUCTION_OPERAND_MAX_VALUE = 1 << (sizeof(unsigned char) << 3);


// between [0,NUM_REGISTERS) - register index
// between [NUM_REGISTERS,INSTRUCTION_OPERAND_SIZE] - RAM address


constexpr unsigned char NO_LABEL = 0;


struct Instruction
{
    unsigned char label_index;
    Op op;
    std::optional<unsigned char> src1;
    std::optional<unsigned char> src2;
    std::optional<unsigned char> dst;

    bool src1_label = false;
    bool src2_label = false;
    bool dst_label  = false;
};


class Assembly
{
    std::vector<Instruction> instructions;
public:
    auto &get_Instructions()       const { return instructions; }

    void add_Instruction(const Instruction &instruction)
    {
        instructions.push_back(instruction);
    }

    void add_ConstDefinition(unsigned int value);
};


void assemble(std::istream &code, std::ostream &output, std::vector<std::string> &messages);


#endif
