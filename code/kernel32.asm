bits 32

main32:
    ; initialize stack and data segment
    mov ax, DATA_SEGMENT
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

    ; setting paging
    

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

halt:
    cli
    hlt
