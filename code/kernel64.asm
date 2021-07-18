bits 64

main64:
    mov rdi, TEXT_DISPLAY              ; Set the destination index to 0xB8000.
    mov rax, 0x1F201F201F201F20   ; Set the A-register to 0x1F201F201F201F20.
    mov rcx, 1000                  ; Set the C-register to 500.
;    mov esi, msg_kernel_success
;    call puts
;    jmp halt
    cld
    rep stosd                     ; Clear the screen.
    hlt                           ; Halt the processor.