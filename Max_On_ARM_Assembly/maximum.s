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
    // load arr address to r1
    adr r1, arr
    // load size to r2
    ldr r2, size

    bl max
    // hang
    b .

// function that returns max number in arr
.global max
max:
    // go to max.found if r2 == 0
    movs r2, r2
    ble max.found

    // load current elelement from arr and point to next element
    // r0 will hold the maximum but currently is just the first element
    ldr r0, [r1], #4

max.loop:
    // decrement arr size and go to max.found if become 0
    subs r2, #1
    ble max.found

    // load current elelement from arr and point to next element
    // r3 will hold the current element
    ldr r3, [r1], #4
    // compare r0 and r3
    cmp r0, r3
    // if (r0 < r3) r3 = r0
    movlt r0, r3
    // loop
    b max.loop

max.found:
    bx lr

