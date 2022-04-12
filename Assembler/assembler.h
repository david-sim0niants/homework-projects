#ifndef __ASSEMBLER_H__
#define __ASSEMBLER_H__

#include <istream>
#include <ostream>
#include <variant>
#include <vector>
#include <unordered_map>



enum class Op
{
    NONE, ADD, SUB, OR, NOT, AND, XOR, MUL, JE, JNE, JLT, JLE, JGT, JGE, MOV
};


constexpr unsigned int NUM_REGISTERS = 16;
constexpr unsigned int NUM_GP_REGISTERS = NUM_REGISTERS - 2;
constexpr unsigned int IO_REG_INDEX = NUM_GP_REGISTERS;
constexpr unsigned int COUNTER_INDEX = NUM_GP_REGISTERS + 1;
constexpr unsigned int INSTRUCTION_OPERAND_SIZE = sizeof(unsigned char);


// between [0,NUM_REGISTERS) - register index
// between [NUM_REGISTERS,INSTRUCTION_OPERAND_SIZE] - RAM address
struct MemoryLocation { unsigned char value; };

struct Immediate      { unsigned int  value; };

struct Label          { unsigned int  value; };

struct Constant       { unsigned int  value; };


using SrcType = std::variant<MemoryLocation, Immediate, Label, Constant>;
using DstType = std::variant<MemoryLocation, Label>;


constexpr unsigned int NO_LABEL = INSTRUCTION_OPERAND_SIZE;


struct Instruction
{
    unsigned int label_index = NO_LABEL;
    Op op;
    SrcType src1;
    SrcType src2;
    DstType dst;
};


class Assembly
{
    std::vector<Instruction> instructions;
    std::unordered_map<unsigned int, unsigned int> constant_to_index_map;
public:
    auto &get_Instructions()       const { return instructions; }
    auto &get_ConstantToIndexMap() const { return constant_to_index_map; }

    void add_Instruction(const Instruction &instruction);
    void add_ConstDefinition(unsigned int value);
};


void assemble(std::istream &code, std::ostream &output);


#endif
