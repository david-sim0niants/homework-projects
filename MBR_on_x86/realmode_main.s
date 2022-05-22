.code16
.section .text
.extern print

.globl init_RealMode
.type init_RealMode, @function

init_RealMode:
    mov $msg, %si
    call print
    hlt

msg:
    .asciz "Real mode init.\r\n"

