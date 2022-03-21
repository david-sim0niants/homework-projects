import sys


def reverse_bits(number):
    return int('{:0b}'.format(number)[::-1], 2)


def generate_operator(bits : int, op = lambda x, y: x + y, empty_cell='.', seperator_cell='*'):

    lower_bits_filter = (1 << bits) - 1
    def get_reversed_ans_state(state):
        ans_state_val = op((state >> bits), (state & lower_bits_filter))
        return reverse_bits(ans_state_val) + (1 << ans_state_val.bit_length())

    symbol_set = f"{empty_cell} {seperator_cell} 0 1\n"

    num_last_states = 4 ** bits
    state_diagram = f'{empty_cell},HALT,r | {seperator_cell},0,r | 0,2,r | 1,3,r\n{empty_cell},HALT,r | {seperator_cell},1,r | 0,2,r | 1,3,r\n'

    for bit in range(1, 2 * bits - 1):
        for state, state_val in zip(range(1 << bit, 1 << (bit + 1)), range(1 << bit)):
            output_symbol = state_val % 2
            output_state = state >> 1
            new_state_0 = state << 1
            new_state_1 = (state << 1) | 1

            state_diagram += f"{output_symbol},{output_state},r | {seperator_cell},{state},r | 0,{new_state_0},r | 1,{new_state_1},r\n"


    for state, state_val in zip(range(num_last_states >> 1, num_last_states), range(num_last_states >> 1)):
        ans_state_0 = get_reversed_ans_state(state_val << 1)
        ans_state_1 = get_reversed_ans_state((state_val << 1) | 1)

        output_symbol = state_val % 2
        output_state = state >> 1

        state_diagram += f"{output_symbol},{output_state},r | {seperator_cell},{state},r | 0,{ans_state_0},r | 1,{ans_state_1},r\n"


    for state, state_val in zip(range(num_last_states, num_last_states << 1), range(num_last_states)):
        output_symbol = state_val % 2
        output_state = state >> 1

        state_diagram += \
        f"{output_symbol},{output_state},r | {seperator_cell},{state},r | {output_symbol},{output_state},r | {output_symbol},{output_state},r\n"


    state_set = ''
    for i in range(num_last_states << 1):
        state_set += f'{i}, '
    state_set += 'HALT\n'
    state_diagram += f"{empty_cell},HALT,r | {seperator_cell},HALT,r | 0,HALT,r | 1,HALT,r\n"

    return symbol_set + state_set + state_diagram


'''
Usage:
python3 generator.py <number of bits> <at least first letter of one of the adder, multiplier, divider operators> <config file to write to>
'''


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Not enough arguments.")
        exit()
    else:
        bits = int(sys.argv[1])

        possible_ops = [lambda x, y: x + y, lambda x, y: x * y, lambda x, y: x // y if y != 0 else 0]
        opname = sys.argv[2]
        op_index = 0 if opname.startswith('a') else 1 if opname.startswith('m') else 2
        op = possible_ops[op_index]

        filename = sys.argv[3]

        with open(filename, 'w') as f:
            # out_bits = [bits + 1, 2 * bits, bits]
            f.write(generate_operator(bits, op))
