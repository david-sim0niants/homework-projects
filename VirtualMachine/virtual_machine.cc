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


uint32_t VirtualMachine::get_SrcValue(uint8_t src)
{
    uint32_t val;
    if (src < NUM_GP_REGISTERS)
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


void VirtualMachine::exec()
{
    if (counter + 4 > memory.size())
    {
        throw mem_out_of_bounds_error("Got out of bounds of memory while trying to read the next instruction.");
    }
    Instruction instruction { memory[counter++], memory[counter++], memory[counter++], memory[counter++] };

    uint32_t src1_val;
    if (instruction.opcode & FIRST_IMMEDIATE)
    {
        src1_val = instruction.src1;
    }
    else
    {
        src1_val = get_SrcValue(instruction.src1);
    }

    uint32_t src2_val;
    if (instruction.opcode & SECOND_IMMEDIATE)
    {
        src2_val = instruction.src2;
    }
    else
    {
        src2_val = get_SrcValue(instruction.src2);
    }

    if (instruction.opcode & 32)
    {
        uint8_t cond_op_index = instruction.opcode & ~FIRST_IMMEDIATE & ~SECOND_IMMEDIATE & (~32);
        uint32_t dst_val = get_SrcValue(instruction.dst);

        switch (cond_op_index)
        {
        case 0:
            if (src1_val == src2_val)
            {
                counter = dst_val;
            }
            break;
        case 1:
            if (src1_val != src2_val)
            {
                counter = dst_val;
            }
            break;
        case 2:
            if (src1_val < src2_val)
            {
                counter = dst_val;
            }
            break;
        case 3:
            if (src1_val <= src2_val)
            {
                counter = dst_val;
            }
            break;
        case 4:
            if (src1_val > src2_val)
            {
                counter = dst_val;
            }
            break;
        case 5:
            if (src1_val >= src2_val)
            {
                counter = dst_val;
            }
            break;
        default:
            throw invalid_opcode_error("Invalid opcode - " + std::to_string(instruction.opcode));
        }
    }
    else
    {
        uint32_t alu_op_index = (instruction.opcode & ~FIRST_IMMEDIATE & ~SECOND_IMMEDIATE);
        uint32_t alu_result;

        switch (alu_op_index)
        {
        case 0:
            alu_result = src1_val + src2_val;
            break;
        case 1:
            alu_result = src1_val - src2_val;
            break;
        case 2:
            alu_result = src1_val & src2_val;
            break;
        case 3:
            alu_result = src1_val | src2_val;
            break;
        case 4:
            alu_result = ~src1_val;
            break;
        case 5:
            alu_result = src1_val ^ src2_val;
            break;
        case 6:
            alu_result = src1_val * src2_val;
            break;
        default:
            throw invalid_opcode_error("Invalid opcode - " + std::to_string(instruction.opcode));
        }

        set_DstValue(instruction.dst, alu_result);
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

