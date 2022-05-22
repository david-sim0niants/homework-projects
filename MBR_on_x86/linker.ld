SECTIONS
{
    . = 0x7c00;
    .boot : { build/boot.o }
    /DISCARD/ :
    {
        *(.note.gnu.property)
    }
    .text : { * }
    .data : { * }
    .bss  : { * }
}
