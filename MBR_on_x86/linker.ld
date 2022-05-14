SECTIONS
{
    . = 0x7C00;
    .boot : { build/boot.o }
    .text : { *(.text) }
    .data : { *(.data) }
    .bss  : { *(.bss)  }
}
