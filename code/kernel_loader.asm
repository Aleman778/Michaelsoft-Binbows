section .loader
bits 16
;
; Starting in 16-bit real mode, the kernel loader is responsible for
; setting up the CPU to run in 64-bit mode and start the kernel.
;

%define ENDL 0x0D, 0x0A

global loader
loader:
    ; enable A20
    ; TODO(alexander): this is not going to work on all computers
    in al, 0x92
    or al, 0x2
    out 0x92, al

    ; set VGA to be normal mode
    mov ax, 0x3
    int 0x10

    ; load the global discriptor table
    cli
    lgdt [gdt.pointer]

    ; set protected mode bit
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    ; jump to 32-bit protected mode of the kernel
    jmp gdt.code:loader32

    ; should never be executed
    jmp halt

; 32-bit global descriptor table
gdt:
.null: equ $ - gdt
    dq 0x0
.code: equ $ - gdt
    dw 0xffff     ; segment limit
    dw 0x0        ; base address (bit 0-15)
    db 0x0        ; base address (bit 16-23)
    db 0b10011010 ; access byte
    db 0b11001111 ; flags
    db 0x0        ; base address (bit 25-31)
.data: equ $ - gdt
    dw 0xffff     ; segment limit
    dw 0x0        ; base address (bit 0-15)
    db 0x0        ; base address (bit 16-23)
    db 0b10010010 ; access byte
    db 0b11001111 ; flags
    db 0x0        ; base address (bit 25-31)
.pointer:
    dw $ - gdt - 1
    dd gdt


bits 32
;
; From 32-bit protected mode the CPU needs to setup
; global descriptor table, CPUID and PAE paging to then
; run the CPU in long (64-bit) mode.
;

edit_gdt_to_64_bit:
    ; modify the access byte to support 64-bit
    mov [gdt + gdt.code + 6], byte 0b10101111
    mov [gdt + gdt.data + 6], byte 0b10101111
    ret

loader32:
    ; initialize stack and data segment
    mov ax, gdt.data 
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

.check_cpuid:
    ; check that CPUID is supported
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    xor eax, ecx
    jz .no_cpuid

    ; check if long (64-bit) mode is supported
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jl .no_long_mode

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode

.setup_pae_paging:
    ; clear PAE paging tables
    mov edi, 0x1000 ; PML4T -> 0x1000
    mov cr3, edi
    xor eax, eax
    mov ecx, 4096
    std             ; makes sure to decrement ecx
    rep stosd
    mov edi, cr3
   
    ; setup page tables
    mov dword [edi], 0x2003 ; PDPT -> 0x2000
    add edi, 0x1000
    mov dword [edi], 0x3003 ; PDT -> 0x3000
    add edi, 0x1000
    mov dword [edi], 0x4003 ; PT -> 0x4000
    add edi, 0x1000


    ; NOTE(alexander): the 3 means that page is present and readable
    mov ebx, 0x00000003
    mov ecx, 512
.set_page_entry:
    mov dword [edi], ebx
    add ebx, 0x1000
    add edi, 8
    loop .set_page_entry
    
    ; enable PAE paging
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

; TODO(alexander): investigate level-5 paging here PML5
.set_pml5_paging:
    mov eax, 0x7
    xor ecx, ecx
    cpuid
    test ecx, (1<<16)
    jz .skip_pml5_paging
    mov eax, cr4
    or eax, (1<<12) ;CR4.LA57
    mov cr4, eax
.skip_pml5_paging:

.switch_to_long_mode:
    ; set LM-bit
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; edit the gdt to support 64-bit paging
    call edit_gdt_to_64_bit

    ; jump to kernel in 64-bit mode
    lgdt [gdt.pointer]
    jmp gdt.code:loader64

.kernel32_success:
    ; kernel was initialized sucessfully
    mov esi, msg_kernel_success
    call puts
    jmp halt

.no_cpuid:
    ; no CPUID support
    mov si, msg_no_cpuid
    call puts
    jmp halt

.no_long_mode:
    ; no long mode support
    mov si, msg_no_long_mode
    call puts
    jmp halt

; 
; Prints null-terminated string from pointer.
; Parameters:
;     - ds:si: points to null-terminated string
;
puts:
    push esi
    push ebx
    push eax
    mov ebx, [buffer_display]
.loop:
    lodsb
    or al, al
    jz .exit
    or ax, 0x0300 ; TODO(alexander): color blue parameterize maybe?

    mov word [ebx], ax
    add ebx, 2
    jmp .loop
.exit:
    mov [buffer_display], ebx ; TODO(alexander): check buffer overflow
    pop eax
    pop ebx
    pop esi
    ret


bits 64
;
; Now the processor is setup to run in long (64-bit) mode.
; The task is now to start the actual kernel which lives in C land.
;

%macro PUSH_ALL_REGS 0
    push rax
    push rcx
    push rdx
    push r8
    push r9
    push r10
    push r11
%endmacro

%macro POP_ALL_REGS 0
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdx
    pop rcx
    pop rax
%endmacro

extern _idt
idt_descriptor:
    dw 4095 ; size of the IDT
    dq _idt

extern isr1_handler
isr1:
    PUSH_ALL_REGS
    call isr1_handler
    POP_ALL_REGS
    iretq
    GLOBAL isr1
    
load_idt:
    lidt[idt_descriptor]
    sti
    ret
    GLOBAL load_idt

extern main
loader64:
    mov rdi, TEXT_DISPLAY
    mov rax, 0x1F201F201F201F20
    mov rcx, 1000
    cld
    rep stosd
    mov esi, msg_kernel_success
    call puts

    ; run the kernel main function in C
    mov esp, kernel_stack_top
    call main

global halt
halt:
    cli
    hlt

msg_kernel_success:
    db "Kernel was initialized successfully!", ENDL, 0

msg_no_cpuid:
    db "No support for CPUID!", ENDL, 0

msg_no_long_mode:
    db "No support for long (64-bit) mode!", ENDL, 0

TEXT_DISPLAY equ 0xb8000
buffer_display:
    dd TEXT_DISPLAY

times 2048-($-$$) db 0 ; end of the kernel loader

; reserve 16K of stack space after the code
section .bss
bits 64
align 8
kernel_stack_bottom: equ $
    resb 16384
kernel_stack_top:
