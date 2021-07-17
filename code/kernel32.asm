bits 32

main32:
    ; initialize stack and data segment
    mov ax, DATA_SEGMENT
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esi, msg_kernel_success
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
