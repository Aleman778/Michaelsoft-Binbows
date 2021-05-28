
PROGRAM_SPACE equ 0x7e00

read_disk:
    mov ah, 0x02 ; bios function for reading disk
    mov bx, PROGRAM_SPACE
; TODO(alexander): this might need to be increased later
    mov al, 4 ; number of sectors (about 2000 bytes) 
    mov dl, [BOOT_DISK]
    mov ch, 0x00 ; cylinder
    mov ch, 0x00 ; head
    mov cl, 0x02 ; sector
    
    int 0x13 ; reads the disk (sets carry flag if failed)
    jc report_disk_read_failed
    
    ret

report_disk_read_failed:
    mov bx, DISK_READ_ERROR_STRING
    call print_string
    jmp $

BOOT_DISK:
    db 0

DISK_READ_ERROR_STRING:
    db "Disk read failed!", 0
