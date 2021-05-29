org 0x7e00
bits 16

mov si, EXTERNAL_PROGRAM_LOADED_SUCCESSFULLY
call puts

jmp $

%include "basic.asm"

EXTERNAL_PROGRAM_LOADED_SUCCESSFULLY:
    db "Loaded the external program successfully!", ENDL, 0

times 2048-($-$$) db 0