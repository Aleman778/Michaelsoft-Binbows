; NOTE: this is running 16-bit real mode

;
; Read sectors from disk
; Parametes:
;     - ax: LBA address
;     - cl: number of sectors to read (max 128)
;     - dl: drive number
;     - es:bx: memory address where to store read data
;
disk_read:
    push ax
    push bx
    push cx
    push dx
    push di

    push cx         ; temporarily save cl
    call lba_to_chs ; compute CHS address
    pop ax          ; al = number of sectors to read (LBA is nolonger needed)
    
    mov ah, 02h
    mov di, 3       ; retry count

.retry:
    pusha           ; save all registers, we can't know what BIOS modifies
    stc             ; set carry flag, some BIOSes don't set it
    int 13h         ; success if carry flag is clearead
    jnc .done

    ; read failed
    popa
    call disk_reset

    dec di
    test di, di
    jnz .retry
    jmp disk_read_error ; all ettempts are exhausted

.done:
    popa
    pop di
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; 
; Resets disk controller
; Parameters:
;     - dl: drive number
;
disk_reset:
    pusha
    mov ah, 0
    stc
    int 13h
    jc disk_read_error
    popa
    ret

disk_read_error:
    mov si, msg_disk_read_failure
    call puts
    jmp wait_key_and_reboot


;
; Converts an LBA (Local Block Addressing) address to CHS (Cylinder-Head-Sector) address
; Parameters:
;     - ax: LBA address
; Returns:
;     - cx (bits 0-5): sector number
;     - cx (bits 6-15): cylinder
;     - dh: head
;
lba_to_chs:
    push ax
    push dx

    xor dx, dx
    div word [bpb_sectors_per_track] ; ax = LBA / sectors_per_track
                                     ; dx = LBA % sectors_per_track

    inc dx                           ; dx = (LBA % sectors_per_track + 1) = sector
    mov cx, dx                       ; cx = sector

    xor dx, dx                       ; dx = 0
    div word [bpb_heads]             ; ax = (LBA / sectors_per_track) / heads = cylinder
                                     ; dx = (LBA / sectors_per_track) % heads = head
    mov dh, dl                       ; dh = head
    mov ch, al                       ; ch = cylinder (lower 8-bits)
    shl ah, 6
    or cl, ah                        ; put upper 2 bits of cylinder in cl

    pop ax
    mov dl, al                       ; restore dl, since dh is occupied by the head
    pop ax
    ret
    
kernel_boot_disk:
    db 0

msg_disk_read_failure:
    db "Disk read failure!", ENDL, 0
