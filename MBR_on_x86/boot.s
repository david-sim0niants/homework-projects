.code16
.section .boot
.extern init_RealMode

_start:
    # disable interrupts
    cli
    # set segment registers (except cs for a strange reason) to zero
    xor %ax, %ax
    mov %ax, %ds
    mov %ax, %ss
    mov %ax, %es
    mov %ax, %bp

    # copy this MBR code to a location 0x600
    # all the addresses here will be offseted by 0x600 by linker.ld script
    mov $0x0100, %cx
    mov $0x7c00, %si
    mov $0x0600, %di
    rep movsw

    # continue in a newly copied code
    jmp _continue

_continue:
    # enable interrupts
    sti
    # set stack pointer
    mov $0x0600, %sp

    # print "Hello world!"
    mov $msg, %si
    call print
    hlt

.globl print
.type print, @function

print:
    # enable 'write in tty' mode in BIOS
    mov $0xE, %ah

.L_print_loop:
    # load and print string bytewise
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

