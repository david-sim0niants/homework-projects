.section .vector_table, "x"
.global _Reset
_Reset:
    b Reset_Handler
    b . /* 0x4 Undefined Instruction */
    b . /* 0x8 Software Interrupt */
    b . /* 0xC Prefetch Abort */
    b . /* 0x10 Data Abort */
    b . /* 0x14 Reserved */
    b . /* 0x18 IRQ */
    b . /* 0x1C FIQ */


.section .text

arr: .word 0x21, 0x45, 0x4252, 0x420, 0x10, 0x2424, 0x2343, 0x340
size: .word 8

Reset_Handler:
    adr r1, arr
    ldr r2, size
    bl max
    b .


.global max
max:
    movs r2, r2
    ble max.found

    ldr r0, [r1], #4

max.loop:
    subs r2, #1
    ble max.found

    ldr r3, [r1], #4
    cmp r0, r3
    movlt r0, r3
    b max.loop

max.found:
    bx lr

