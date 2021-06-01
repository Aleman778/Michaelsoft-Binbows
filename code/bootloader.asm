org 0x7c00
bits 16

;
; FAT12 - BPB (BIOS Parameter Block)
; https://wiki.osdev.org/FAT#BPB_.28BIOS_Parameter_Block.29
;
jmp short start
nop

bpb_oem:                   db "MSWIN4.1" ; 8 bytes
bpb_bytes_per_sector:      dw 512
bpb_sectors_per_cluster:   db 1
bpb_reserved_sectors:      dw 1
bpb_fat_count:             db 2
bpb_dir_entry_count:     dw 0x0E0
bpb_total_sectors:         dw 2880 ; 2880 * 512 = 1.44MB
bpb_media_discriptor_type: db 0xF0 ; 3.5" floppy disk
bpb_sectors_per_fat:       dw 9    ; 9 sectors/fat
bpb_sectors_per_track:     dw 18
bpb_heads:                 dw 2
bpb_hidden_sectors:        dd 0
bpb_large_sector_count:    dd 0

; extended boot record
ebr_drive_number: db 0                  ; 0x00 floppy, 0x80 hdd
                  db 0                  ; reserved
ebr_signature:    db 29h
ebr_volume_id:    db 12h, 34h, 56h, 78h ; serial number, 
ebr_volume_label: db "binbows os "      ; 11 bytes, padded with spaces
ebr_system_id:    db "FAT12   "         ; 8 bytes

start:
    jmp main

%include "basic.asm"
%include "disk_read.asm"

;
; Bootloader entry point.
;
main:
    ; setup data segments
    mov ax, 0 ; cannot write directly to segment registers
    mov ds, ax
    mov es, ax

    ; setup the stack
    mov ss, ax
    mov bp, 0x7c00 ; stack grows downwards, so it will not overwrite this code
    mov sp, bp

    ; load the kernel from floppy disk
    mov [ebr_drive_number], dl   ; dl should be set by the BIOS
    mov ax, 1                    ; LBA = 1, second sector from disk
    mov cl, 4                    ; number of sectors to read
    mov bx, kernel_disk_location ; kernel is stored after the bootloader
    call disk_read
    jmp kernel_disk_location

    mov si, msg_hello_world
    call puts

    jmp .halt

kernel_disk_location equ 0x7e00 ; the location of the kernel program in disk

.halt:
    cli  ; disable interrupts, make sure CPU cannot exit halt state
    hlt

msg_hello_world:
    db "Hello World!", ENDL, 0

times 510-($-$$) db 0
dw 0xaa55
