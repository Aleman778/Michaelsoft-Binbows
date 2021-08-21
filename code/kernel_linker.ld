ENTRY(loader)
OUTPUT_FORMAT("binary")

SECTIONS {
    . = 0x8000;
    .text : {
        *(.loader)
        *(.text)
    }

    .idt BLOCK(0x1000) : ALIGN(0x1000) {
        _idt = .;
        . = . + 0x1000;
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