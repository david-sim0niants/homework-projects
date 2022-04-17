start:
    MOV r14, r0
    MOV r14, r1
    MUL r0, r1, r2
    MOV r2, r14

    JGT r2, #10, start
