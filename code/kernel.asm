bits 16


mov si, msg_kernel_loaded_successfully
call puts

jmp wait_key_and_reboot

%include "basic.asm"

msg_kernel_loaded_successfully:
    db "Loaded the kernel successfully!", ENDL, 0

times 2048-($-$$) db 0