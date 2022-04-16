#ifndef __ASSEMBLER_H__
#define __ASSEMBLER_H__


#include <istream>
#include <ostream>
#include <unordered_map>
#include <vector>
#include <variant>


enum class Mnemonic
{
    NONE, ADD, SUB, OR, NOT, AND, XOR, MUL, JE, JNE, JLT, JLE, JGT, JGE, JMP, MOV
};


using OperandMemLoc = uint32_t;
using OperandImmediate = int32_t;
using OperandStr = std::string;


using SrcOperand = std::variant<std::monostate, OperandImmediate, OperandMemLoc, OperandStr>;
using DstOperand = std::variant<std::monostate, OperandMemLoc, OperandStr>;


struct Instruction
{
    Mnemonic mnemonic;
    SrcOperand src1;
    SrcOperand src2;
    DstOperand dst;

    size_t size() const
    {
        return 0x4;
    }
};


struct Assembly
{
    std::vector<Instruction> instructions;
    std::unordered_map<std::string, int32_t> labels;
};


constexpr unsigned int NUM_GP_REGISTERS = 13;
constexpr unsigned int IO_REGISTER_INDEX = NUM_GP_REGISTERS + 1;
constexpr unsigned int COUNTER_INDEX = NUM_GP_REGISTERS + 2;
constexpr unsigned int NUM_REGISTERS = NUM_GP_REGISTERS + 3;


void parse_Assembly(std::istream &input, Assembly &parsed_assembly, std::vector<std::string> &messages);
void assemble(const Assembly &assembly, std::ostream &output, std::vector<std::string> &messages);
void assemble(std::istream &input, std::ostream &output, std::vector<std::string> &messages);


#endif
