bits 16
org 0x1000

%define ENDL 0x0D, 0x0A

main:    
    ; enable A20
    ; TODO(alexander): this is not going to work on all computers
    in al, 0x92
    or al, 0x2
    out 0x92, al

    ; set VGA to be normal mode
    mov ax, 0x3
    int 0x10

    cli                ; clear interrupts
    lgdt [gdt_pointer] ; load global/ interrupt descriptor table register

    ; set protected mode bit
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    ; jump to 32-bit protected mode of the kernel
    jmp CODE_SEGMENT:main32

    ; should never be executed
    jmp halt

;
; Global Descriptor Table:
; |31                   24|23                   16|15                    8|7                     0|
; |-----------------------|---|----|---|---|------|---|---|------|------|-------------------------|
; | base address (24-31)  | G | DB | - | A | limit| P | D | DPL  | type | base address (16-23)    |
; |-----------------------|---|----|---|---|------|---|---|------|------|-------------------------|
; | base address (0-15)                           | segment limit (0-15)                          |
; |-----------------------------------------------|-----------------------------------------------|
;
; TODO(alexander): document each of the entries in the descriptor table here
;

gdt_start:
    dq 0x0
gdt_code:
    dw 0xffff     ; segment limit
    dw 0x0        ; base address (bit 0-15)
    db 0x0        ; base address (bit 16-23)
    db 0b10011010 ; access byte
    db 0b11001111 ; flags
    db 0x0        ; base address (bit 25-31)
gdt_data:
    dw 0xffff     ; segment limit
    dw 0x0        ; base address (bit 0-15)
    db 0x0        ; base address (bit 16-23)
    db 0b10010010 ; access byte
    db 0b11001111 ; flags
    db 0x0        ; base address (bit 25-31)
gdt_end:
gdt_pointer:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEGMENT equ gdt_code - gdt_start
DATA_SEGMENT equ gdt_data - gdt_start

%include "kernel32.asm"

msg_kernel_success:
    db "Kernel was initialized successfully!", 0

TEXT_DISPLAY equ 0xb8000
buffer_display:
    dd TEXT_DISPLAY

times 2048-($-$$) db 0