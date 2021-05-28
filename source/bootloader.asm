[org 0x7c00]

mov bp, 0x7c00
mov sp, bp

mov bx, test_string
call print_string

jmp $

print_string:
    mov ah, 0x0e
    .loop:
        cmp [bx], byte 0
        je .exit
        mov al, [bx]
        int 0x10
        inc bx
        jmp .loop
    .exit:
        ret

test_string:
    db 'This is a test string!', 0

times 510-($-$$) db 0
dw 0xaa55
