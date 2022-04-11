#include "virtual_machine.h"

#include <cstdio>
#include <iterator>


VirtualMachine::VirtualMachine(size_t mem_size, std::istream *input,
                               std::ostream *output, uint32_t counter_val)
: memory(mem_size), input(input), output(output), counter(counter_val), registers({}) {}


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


std::pair<uint8_t, uint8_t> VirtualMachine::get_SrcValues(Instruction inst)
{
    uint8_t val1;
    if (inst.opcode & FIRST_IMMEDIATE)
    {
        val1 = inst.src1;
    }
    else if (inst.src1 < NUM_GP_REGISTERS)
    {
        val1 = registers[inst.src1];
    }
    else if (inst.src1 == IO_REG_INDEX)
    {
        if (input)
        {
            *input >> val1;
        }
        else
        {
            val1 = 0;
        }
    }
    else if (inst.src1 == COUNTER_INDEX)
    {
        val1 = counter;
    }
    else
    {
        size_t addr = inst.src1;
        if (addr + 4 > memory.size())
        {
            throw mem_out_of_bounds_error("Couldn't read memory at address out of memory bounds.");
        }
        else
        {
            val1  = memory[addr++] << 24;
            val1 |= memory[addr++] << 16;
            val1 |= memory[addr++] << 8;
            val1 |= memory[addr++];
        }
    }

    uint8_t val2;
    if (inst.opcode & FIRST_IMMEDIATE)
    {
        val2 = inst.src2;
    }
    else if (inst.src2 < NUM_GP_REGISTERS)
    {
        val2 = registers[inst.src2];
    }
    else if (inst.src2 == IO_REG_INDEX)
    {
        if (input)
        {
            *input >> val2;
        }
        else
        {
            val2 = 0;
        }
    }
    else if (inst.src2 == COUNTER_INDEX)
    {
        val2 = counter;
    }
    else
    {
        size_t addr = inst.src2;
        if (addr + 4 > memory.size())
        {
            throw mem_out_of_bounds_error("Couldn't read memory at address out of memory bounds.");
        }
        else
        {
            val2  = memory[addr++] << 24;
            val2 |= memory[addr++] << 16;
            val2 |= memory[addr++] << 8;
            val2 |= memory[addr++];
        }
    }

    return std::make_pair(val1, val2);
}


void VirtualMachine::set_DstValue(uint8_t dst, uint8_t value)
{
}


void VirtualMachine::exec()
{
    if (counter + 4 > memory.size())
    {
        counter = 0;
    }
    Instruction instruction { memory[counter++], memory[counter++], memory[counter++], memory[counter++] };

    auto src_values = get_SrcValues(instruction);

}

