#ifndef __ASSEMBLER_H__
#define __ASSEMBLER_H__

#include <istream>
#include <ostream>
#include <variant>
#include <vector>
#include <unordered_map>



enum OpType
{
    ADD, SUB, OR, NOT, AND, XOR, MUL, JE, JNE, JLT, JLE, JGT, JGE, MOV
};


constexpr unsigned int NUM_REGISTERS = 16;
constexpr unsigned int NUM_GP_REGISTERS = NUM_REGISTERS - 2;
constexpr unsigned int IO_REG_INDEX = NUM_GP_REGISTERS;
constexpr unsigned int COUNTER_INDEX = NUM_GP_REGISTERS + 1;
constexpr unsigned int INSTRUCTION_OPERAND_SIZE = sizeof(unsigned char);


/*
 MemoryLocation
 between [0,15] - register
 between [16,INSTRUCTION_OPERAND_SIZE) - 8bit memory address
 between [INSTRUCTION_OPERAND_SIZE,) - index of label appearing in assembly
*/
using MemoryLocation = unsigned int;
/*
 ImmediateValue
 between [0,INSTRUCTION_OPERAND_SIZE) - immediate value
 between [INSTRUCTION_OPERAND_SIZE,)  - index of constant appearing in assembly
*/
using ImmediateValue = unsigned int;


using SrcType = std::variant<MemoryLocation, ImmediateValue>;
using DstType = MemoryLocation;


constexpr unsigned int NO_LABEL = INSTRUCTION_OPERAND_SIZE;


struct Instruction
{
    unsigned int label_index = NO_LABEL;
    OpType op;
    SrcType src1;
    SrcType src2;
    DstType dst;
};


class Assembly
{
    std::vector<Instruction> instructions;
    std::unordered_map<unsigned int, unsigned int> constant_to_index_map;
};


void assemble(std::istream &code, std::ostream &output);


#endif
