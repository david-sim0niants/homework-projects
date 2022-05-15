SECTIONS
{
    . = 0x0600;
    .boot : { build/boot.o }
    .text : { *(.text) }
    .data : { *(.data) }
    .bss  : { *(.bss)  }
}
