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
bpb_dir_entry_count:       dw 0E0h
bpb_total_sectors:         dw 2880 ; 2880 * 512 = 1.44MB
bpb_media_discriptor_type: db 0F0h ; 3.5" floppy disk
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
ebr_volume_label: db "BINBOWS OS "      ; 11 bytes, padded with spaces
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

    ; Some BIOSes might start us with code segment 0x07C0
    push es
    push word .after
    retf

.after:
    mov [ebr_drive_number], dl ; dl should be set by the BIOS

    ; show loading message
    mov si, msg_loading
    call puts

    ; read drive parameters (instead of relying on formatted disk)
    push es
    mov ah, 08h
    int 13h
    jc disk_read_error
    pop es

    and cl, 0x3f
    xor ch, ch
    mov [bpb_sectors_per_track], cx

    inc dh
    mov [bpb_heads], dh

    ; read FAT root directory
    mov ax, [bpb_dir_entry_count]
    shl ax, 5
    xor dx, dx
    div word [bpb_bytes_per_sector]
    mov cl, al                          ; cl = number of sectors
    test dx, dx                         ; make sure to read entire partial sector
    jz .skip_partial_sector
    inc cl

.skip_partial_sector:
    mov ax, [bpb_sectors_per_fat]
    mov bl, [bpb_fat_count]
    xor bh, bh
    mul bx
    add ax, [bpb_reserved_sectors] ; ax = lba value

    mov dl, [ebr_drive_number]
    mov bx, buffer
    call disk_read

    ; next buffer location
    ; mov al, cl
    ; xor ah, ah
    ; xor dx, dx
    ; mul word [bpb_bytes_per_cluster]
    ; add bx, ax ; ax = new buffer location

    ; search for the kernel.bin file
    xor bx, bx
    mov di, filename_kernel_bin
    
.search_kernel:
    mov si, buffer
    mov cx, 11
    push di
    repe cmpsb ; NOTE(alexander): doesn't seem to work on null characters, always equals
    pop di
    je .found_kernel
    
    add di, 32
    inc bx
    cmp bx, [bpb_dir_entry_count]
    jl .search_kernel
    jmp kernel_not_found_error

.found_kernel:
    mov si, msg_kernel_found
    call puts
    jmp .halt
     
    ; Read the kernel into memory
    

    ; read FAT
    mov ax, [bpb_reserved_sectors]
    mov cl, [bpb_sectors_per_fat]
    mov dl, [ebr_drive_number]
    call disk_read

.halt:
    cli  ; disable interrupts, make sure CPU cannot escape
    hlt

kernel_not_found_error:
    mov si, msg_kernel_not_found
    call puts
    jmp wait_key_and_reboot

msg_kernel_found:     db "Kernel is found!", ENDL, 0
msg_kernel_not_found: db "KERNEL.BIN is not found!", ENDL, 0
msg_loading:          db "Loading...", ENDL, 0
filename_kernel_bin:  db "KERNEL  BIN"
kernel_cluster:       db 0

times 510-($-$$) db 0
dw 0xaa55

buffer: