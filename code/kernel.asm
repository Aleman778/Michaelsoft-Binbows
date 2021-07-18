bits 16
org 0x8000

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
    lgdt [gdt.pointer] ; load global/ interrupt descriptor table register

    ; set protected mode bit
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    ; jump to 32-bit protected mode of the kernel
    jmp gdt.code:main32

    ; should never be executed
    jmp halt

%include "gdt.asm"
%include "kernel32.asm"
%include "kernel64.asm"

msg_kernel_success:
    db "Kernel was initialized successfully!", ENDL, 0

msg_no_cpuid:
    db "No support for CPUID!", ENDL, 0

msg_no_long_mode:
    db "No support for long (64-bit) mode!", ENDL, 0

TEXT_DISPLAY equ 0xb8000
buffer_display:
    dd TEXT_DISPLAY

times 2048-($-$$) db 0