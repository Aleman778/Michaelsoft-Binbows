
%define ENDL 0x0D, 0x0A
; 
; Prints null-terminated string from pointer.
; Parameters:
;     - ds:si: points to null-terminated string
;
puts:
    push si
    push ax
    mov ah, 0x0e
.loop:
    lodsb ; loads byte to al and increments si
    or al, al ; check if next character is null
    jz .exit
    int 0x10 ; call bios interrupt
    jmp .loop
.exit:
    pop ax
    pop si
    ret
