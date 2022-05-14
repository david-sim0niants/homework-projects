.code16
.globl init_RealMode
.type init_RealMode, @function
.extern print


init_RealMode:
    mov $msg, %si
    call print
    hlt

msg:
    .asciz "Real mode init"
