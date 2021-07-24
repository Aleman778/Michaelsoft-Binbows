ENTRY(loader)
OUTPUT_FORMAT("binary")
SECTIONS {
    . = 0x8000;
    .text : {
        *(.loader)
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
    }
}