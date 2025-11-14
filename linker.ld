/* SolixOS Kernel Linker Script */
ENTRY(kmain)

SECTIONS
{
    . = 0x100000; /* Kernel loads at 1MB */
    
    .text :
    {
        *(.text)
        *(.rodata*)
    }
    
    .data :
    {
        *(.data)
    }
    
    .bss :
    {
        __bss_start = .;
        *(.bss)
        *(COMMON)
        __bss_end = .;
    }
    
    . = ALIGN(4096);
    __end = .;
}
