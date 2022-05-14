.code16

.section .boot


_start:
    cli
    xor %ax, %ax
    mov %ax, %ds
    mov %ax, %ss
    mov %ax, %es
    mov %ax, %bp

    mov $0x0100, %cx
    mov $0x7c00, %si
    mov $0x0600, %di
    rep movsw

    jmpl $0x0600, $_continue

_continue:
    sti
    mov $0x0600, %sp

    mov $msg, %si
    call print
    hlt

.globl print
.type print, @function

print:
    mov $0xE, %ah

.L_print_loop:
    lodsb
    cmp $0, %al
    je .L_print_exit

    int $0x10
    jmp .L_print_loop

.L_print_exit:
    ret

msg: .asciz "Hello world!"


# .org 0x7C00
.zero 510 - (. - _start)
.byte 0x55, 0xAA

