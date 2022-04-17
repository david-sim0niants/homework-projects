// comments only with //
// constants are defined like labels
constant: #12345

label:
// ADD value at address 0x20 (read in hex) and immediate value 20
// and store it in r0 register
    ADD 20, #20, r0
    JGE r0, #10, another_label // jump another_label if r0 >= 10
// inspired from ARM Assembly, you can set value to counter register like this
    MOV #0, pc
// or r15 is the same as program counter
    MOV #0, r15
// multiplicate immediates and store result in r1 register
    MUL #10, #20, r1
    // jump to label
    JMP label

    NOP
    NOP
    NOP
    NOP
another_label:
    JMP another_label
