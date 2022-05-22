.code16
.section .boot
.extern init_RealMode

_start:
    cli
    mov $msg, %si
    call print
    call read_Sector
    jmp init_RealMode
    

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

read_Sector:
    mov $2,  %ah # BIOS 0x13 interrupt, 2-nd function
    mov $63, %al # 63 sectors to read
    mov $0,  %ch # cylinder number 0
    mov $2,  %cl # sector number 2
    mov $0,  %dh # head number 0
    xor %bx, %bx
    mov %bx, %es # set extended segment register to zero
    mov $init_RealMode, %bx # 0x7E00 - address 512 bytes after 0x7C00
                     # where the next sector will be loaded
    int $0x13
    ret

msg: .asciz "Hello world!\r\n"

.zero 510 - (. - _start)
.byte 0x55, 0xAA

