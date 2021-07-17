
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

;
; Waits for any keypress and reboots the computer
;
wait_key_and_reboot:
    mov ah, 0
    int 16h      ; wait for keypress
    jmp 0FFFFh:0 ; jump to beginning of BIOS, should reboot
.halt:
    cli  ; disable interrupts, make sure CPU cannot escape
    hlt
