bits 16

;
; Global Descriptor Table:
; |31                    24|23                    16|15                     8|7                      0|
; |------------------------|---|----|---|---|-------|---|-----|---|----------|------------------------|
; | base address (24-31)   | G | DB | - | A | limit | P | DPL | S | type     | base address (16-23)   |
; |------------------------|---|----|---|---|-------|---|-----|---|----------|------------------------|
; | base address (0-15)                             | segment limit (0-15)                            |
; |-------------------------------------------------|-------------------------------------------------|
;
; TODO(alexander): document each of the entries in the descriptor table here
;

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

edit_gdt_to_64_bit:
    ; modify the access byte to support 64-bit
    mov [gdt.code + 6], byte 0b10101111
    mov [gdt.data + 6], byte 0b10101111
    ret

bits 16