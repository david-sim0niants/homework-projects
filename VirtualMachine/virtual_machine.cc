#include "virtual_machine.h"

#include <cstdio>
#include <string>


VirtualMachine::VirtualMachine(size_t mem_size, std::istream *input,
                               std::ostream *output, uint32_t counter_val)
: memory(mem_size), input(input), output(output), counter(counter_val), gp_registers({}) {}


void VirtualMachine::upload_Program(std::istream &program)
{
    size_t bytes_read = 32;

    auto *mem = memory.data();
    size_t mem_size = memory.size();

    while (bytes_read <= mem_size && program.peek() != EOF)
    {
        program.read(mem, 32);
        bytes_read += 32;
        mem += 32;
    }

    if (program.peek() != EOF)
    {
        throw vm_error("Program size larger than memory limit.");
    }
}


uint32_t VirtualMachine::get_SrcValue(uint8_t opcode, uint8_t src)
{
    uint32_t val;
    if (opcode & FIRST_IMMEDIATE)
    {
        val = src;
    }
    else if (src < NUM_GP_REGISTERS)
    {
        val = gp_registers[src];
    }
    else if (src == IO_REG_INDEX)
    {
        if (input)
        {
            *input >> val;
        }
        else
        {
            val = 0;
        }
    }
    else if (src == COUNTER_INDEX)
    {
        val = counter;
    }
    else
    {
        size_t addr = src;
        if (addr + 4 > memory.size())
        {
            throw mem_out_of_bounds_error("Couldn't read memory at address out of memory bounds.");
        }
        else
        {
            val  = memory[addr++] << 24;
            val |= memory[addr++] << 16;
            val |= memory[addr++] << 8;
            val |= memory[addr++];
        }
    }

    return val;
}


void VirtualMachine::set_DstValue(uint8_t dst, uint32_t value)
{
    if (dst < NUM_GP_REGISTERS)
    {
        gp_registers[dst] = value;
    }
    else if (dst == IO_REG_INDEX)
    {
        if (output)
        {
            *output << value << '\n';
        }
    }
    else if (dst == COUNTER_INDEX)
    {
        counter = value;
    }
    else
    {
        if (dst + 4 >= memory.size())
        {
            throw mem_out_of_bounds_error("Couldn't write at memory address out of memory bounds.");
        }

        memory[dst] = value >> 24;
        memory[dst] = value >> 16;
        memory[dst] = value >> 8;
        memory[dst] = value;
    }
}


uint32_t VirtualMachine::op_ADD(uint32_t src1, uint32_t src2) { return src1 + src2; }
uint32_t VirtualMachine::op_SUB(uint32_t src1, uint32_t src2) { return src1 - src2; }
uint32_t VirtualMachine::op_AND(uint32_t src1, uint32_t src2) { return src1 & src2; }
uint32_t VirtualMachine::op_OR (uint32_t src1, uint32_t src2) { return src1 | src2; }
uint32_t VirtualMachine::op_NOT(uint32_t src1, uint32_t src2) { return ~src1;       }
uint32_t VirtualMachine::op_XOR(uint32_t src1, uint32_t src2) { return src1 ^ src2; }

void VirtualMachine::op_IF_EQ           (uint32_t src1, uint32_t src2, uint32_t dst)
{
    if (src1 == src2)
    {
        counter = dst;
    }
}

void VirtualMachine::op_IF_NOT_EQ       (uint32_t src1, uint32_t src2, uint32_t dst)
{
    if (src1 != src2)
    {
        counter = dst;
    }
}

void VirtualMachine::op_IF_LESS         (uint32_t src1, uint32_t src2, uint32_t dst)
{
    if (src1 < src2)
    {
        counter = dst;
    }
}

void VirtualMachine::op_IF_LESS_OR_EQ   (uint32_t src1, uint32_t src2, uint32_t dst)
{
    if (src1 <= src2)
    {
        counter = dst;
    }
}

void VirtualMachine::op_IF_GREATER      (uint32_t src1, uint32_t src2, uint32_t dst)
{
    if (src1 > src2)
    {
        counter = dst;
    }
}

void VirtualMachine::op_IF_GREATER_OR_EQ(uint32_t src1, uint32_t src2, uint32_t dst)
{
    if (src1 >= src2)
    {
        counter = dst;
    }
}


VirtualMachine::ALU_op VirtualMachine::ALU_ops[6] = {
    op_ADD, op_SUB, op_AND, op_OR, op_NOT, op_XOR
};

VirtualMachine::COND_op VirtualMachine::COND_ops[6] = {
    &VirtualMachine::op_IF_EQ,
    &VirtualMachine::op_IF_NOT_EQ,
    &VirtualMachine::op_IF_LESS,
    &VirtualMachine::op_IF_LESS_OR_EQ,
    &VirtualMachine::op_IF_GREATER,
    &VirtualMachine::op_IF_GREATER
};


void VirtualMachine::exec()
{
    if (counter + 4 > memory.size())
    {
        counter = 0;
    }
    Instruction instruction { memory[counter++], memory[counter++], memory[counter++], memory[counter++] };

    uint32_t src1_val = get_SrcValue(instruction.opcode, instruction.src1);
    uint32_t src2_val = get_SrcValue(instruction.opcode, instruction.src2);

    if (instruction.opcode & 32)
    {
        uint8_t cond_op_index = instruction.opcode & ~FIRST_IMMEDIATE & ~SECOND_IMMEDIATE & (~32);
        if (cond_op_index >= 6)
        {
            throw invalid_opcode_error("Invalid opcode - " + std::to_string(instruction.opcode));
        }

        uint32_t dst_val = get_SrcValue(instruction.opcode, instruction.dst);
        (this->*(COND_ops[cond_op_index]))(src1_val, src2_val, dst_val);
    }
    else
    {
        uint32_t alu_op_index = (instruction.opcode & ~FIRST_IMMEDIATE & ~SECOND_IMMEDIATE);

        if (alu_op_index >= 6)
        {
            throw invalid_opcode_error("Invalid opcode - " + std::to_string(instruction.opcode));
        }

        set_DstValue(instruction.dst, ALU_ops[alu_op_index](src1_val, src2_val));
    }
}


void VirtualMachine::run()
{
    uint32_t prev_counter_val = counter;

    do
    {
        prev_counter_val = counter;
        exec();
    } while (counter != prev_counter_val);
}

