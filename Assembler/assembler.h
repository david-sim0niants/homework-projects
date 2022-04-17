#ifndef __ASSEMBLER_H__
#define __ASSEMBLER_H__


#include <istream>
#include <ostream>
#include <unordered_map>
#include <vector>
#include <variant>


// Mnemonic type
enum class Mnemonic
{
    NONE, ADD, SUB, OR, NOT, AND, XOR, MUL, JE, JNE, JLT, JLE, JGT, JGE, JMP, MOV, NOP
};


// Operand type that holds a register index or an address in RAM
using OperandMemLoc = uint32_t;
// Operand type that holds an immediate value (can be signed)
using OperandImmediate = int32_t;
// Operand type that holds a str (label)
using OperandStr = std::string;


// Source operand variant type
using SrcOperand = std::variant<std::monostate, OperandImmediate, OperandMemLoc, OperandStr>;
// Destination operand variant type
using DstOperand = std::variant<std::monostate, OperandMemLoc, OperandStr>;


// Instruction type
struct Instruction
{
    Mnemonic mnemonic;
    SrcOperand src1;
    SrcOperand src2;
    DstOperand dst;

    // size of instruction
    size_t size() const
    {
        return 0x4;
    }
};


// Map mapping labels to their values (addresses or constants)
using Label2Int_Map = std::unordered_map<std::string, int32_t>;


// Parsed assembly type
struct Assembly
{
    std::vector<Instruction> instructions;
    Label2Int_Map labels;
};


constexpr unsigned int NUM_GP_REGISTERS = 13;
constexpr unsigned int IO_REGISTER_INDEX = NUM_GP_REGISTERS + 1;
constexpr unsigned int COUNTER_INDEX = NUM_GP_REGISTERS + 2;
constexpr unsigned int NUM_REGISTERS = NUM_GP_REGISTERS + 3;
constexpr unsigned int OPERAND_VALUE_LIMIT = 1 << (sizeof(unsigned char) * 8);


// parse assembly from input stream to the parsed type
void parse_Assembly(std::istream &input, Assembly &parsed_assembly, std::vector<std::string> &messages);

// assemble parsed assembly
bool assemble(const Assembly &assembly, std::ostream &output, std::vector<std::string> &messages);

// assemble assembly from input stream to binary output stream
bool assemble(std::istream &input, std::ostream &output, std::vector<std::string> &messages);


#endif
