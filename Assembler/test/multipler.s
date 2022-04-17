start:
    MOV io, r0 // mov input to r0
    MOV io, r1 // mov input to r1
    MUL r0, r1, r2 // multiply r0 and r1, store on r2
    MOV r2, io // mov r2 to output

    // jump to startif r2 greater than 10 (for no reason)
    JGT r2, #10, start
