start:
    MOV io, r0 // mov input to r0
    MOV io, r1 // mov input to r1
    MUL r0, r1, r2 // multiply r0 and r1, store on r2
    MOV r2, io // mov r2 to output

    // jump to start if r2 greater than 010 aka 8 (for no reason)
    JGT r2, #010, start
this:
    JMP this
