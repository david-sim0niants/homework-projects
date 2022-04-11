#ifndef __VIRTUAL_MACHINE_H__
#define __VIRTUAL_MACHINE_H__


#include <istream>
#include <ostream>

#include <cstdint>
#include <stdexcept>
#include <vector>
#include <array>



enum OpCode : uint8_t
{
    ADD = 0,
    SUB = 1,
    AND = 2,
    OR = 3,
    NOT = 4,
    XOR = 5,

    IF_EQ = 32,
    IF_NOT_EQ = 33,
    IF_LESS = 34,
    IF_LESS_OR_EQ = 35,
    IF_GREATER = 36,
    IF_GREATER_OR_EQ = 37
};


constexpr unsigned char FIRST_IMMEDIATE = 64;
constexpr unsigned char SECOND_IMMEDIATE = 128;


struct Instruction
{
    uint32_t opcode :8;
    uint32_t src1   :8;
    uint32_t src2   :8;
    uint32_t dst    :8;

    Instruction(char opcode, char src1, char src2, char dst)
    : opcode(opcode), src1(src1), src2(src2), dst(dst) {}
};



class vm_error : std::logic_error
{
public:
    vm_error(const std::string &msg) : std::logic_error("VM error: " + msg) {}
};


class mem_out_of_bounds_error : vm_error
{
public:
    mem_out_of_bounds_error(const std::string &msg) : vm_error("Memory out of bounds: " + msg) {}
};


class invalid_opcode_error : vm_error
{
public:
    invalid_opcode_error(const std::string &msg) : vm_error("Invalid opcode :" + msg) {}
};



class VirtualMachine
{
public:
    static constexpr size_t NUM_REGISTERS = 16;
    static constexpr size_t NUM_GP_REGISTERS = NUM_REGISTERS - 2;

    static constexpr size_t IO_REG_INDEX = NUM_GP_REGISTERS;
    static constexpr size_t COUNTER_INDEX = NUM_GP_REGISTERS + 1;

private:
    std::vector<char> memory;
    std::array<uint32_t, NUM_REGISTERS> gp_registers;
    uint32_t counter;

    std::istream *input = nullptr;
    std::ostream *output = nullptr;

public:

    explicit VirtualMachine(size_t mem_size, std::istream *input = nullptr,
                   std::ostream *output = nullptr, uint32_t counter_val = 0);

    auto &get_Memory()    const { return memory; }
    auto &get_Registers() const { return gp_registers; }
    auto &get_Counter()   const { return counter; }


    inline void connect_Input(std::istream *input)
    {
        this->input = input;
    }

    inline void connect_Output(std::ostream *output)
    {
        this->output = output;
    }


    void upload_Program(std::istream &program);
    void exec();
    void run();

private:
    uint32_t get_SrcValue(uint8_t opcode, uint8_t src);
    void set_DstValue(uint8_t dst, uint32_t value);

    static uint32_t op_ADD(uint32_t src1, uint32_t src2);
    static uint32_t op_SUB(uint32_t src1, uint32_t src2);
    static uint32_t op_AND(uint32_t src1, uint32_t src2);
    static uint32_t op_OR (uint32_t src1, uint32_t src2);
    static uint32_t op_NOT(uint32_t src1, uint32_t src2);
    static uint32_t op_XOR(uint32_t src1, uint32_t src2);

    void op_IF_EQ           (uint32_t src1, uint32_t src2, uint32_t dst);
    void op_IF_NOT_EQ       (uint32_t src1, uint32_t src2, uint32_t dst);
    void op_IF_LESS         (uint32_t src1, uint32_t src2, uint32_t dst);
    void op_IF_LESS_OR_EQ   (uint32_t src1, uint32_t src2, uint32_t dst);
    void op_IF_GREATER      (uint32_t src1, uint32_t src2, uint32_t dst);
    void op_IF_GREATER_OR_EQ(uint32_t src1, uint32_t src2, uint32_t dst);

    using ALU_op = uint32_t (*) (uint32_t, uint32_t);
    using COND_op = void (VirtualMachine::*) (uint32_t, uint32_t, uint32_t);

    static ALU_op ALU_ops[6];
    static COND_op COND_ops[6];
};


#endif

