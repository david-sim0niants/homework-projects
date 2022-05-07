.code16
.globl _start
.type _global, @function

_start:
    mov $msg, %si
    call print

    mov $another_msg, %si
    call print

print:
    # set "write char in TTY mode" mode for 0x10 BIOS interrupt
    mov $0xE, %ah

.print_loop:
    # load byte from source index %si into %al
    lodsb
    # check null char and in this case jump to exit
    cmp $0, %al
    je .print_exit

    # BIOS interrupt to write char in TTY mode
    int $0x10
    jmp .print_loop

.print_exit:
    ret

msg:
    .asciz "Welcome to MBR World!!"
another_msg:
    .asciz "\n\rMBR successfully loaded."

# fill zeros till reach the 510th byte
.zero 510 - (. - _start)
# MAGIC
.byte 0x55, 0xAA

