[org 0x7e00]

mov bx, EXTERNAL_PROGRAM_LOADED_SUCCESSFULLY
call print_string

jmp $

%include "print.asm"

EXTERNAL_PROGRAM_LOADED_SUCCESSFULLY:
    db "Loaded the external program succesfully!", 0

times 2048-($-$$) db 0