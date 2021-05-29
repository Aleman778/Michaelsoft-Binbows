org 0x7c00
bits 16

start:
    jmp main

%include "basic.asm"
%include "disk_read.asm"

;
; Bootloader entry point.
;
main:
    ; setup data segments
    mov ax, 0 ; cannot write directly to segment registers
    mov ds, ax
    mov es, ax

    ; setup the stack
    mov ss, ax
    mov bp, 0x7c00 ; stack grows downwards, so it will not overwrite this code
    mov sp, bp

    ; load external program from disc
    mov [BOOT_DISK], dl
    call read_disk
    jmp PROGRAM_SPACE

    hlt

.halt:
    jmp .halt

times 510-($-$$) db 0
dw 0xaa55
