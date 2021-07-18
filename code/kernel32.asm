bits 32

main32:
    ; initialize stack and data segment
    mov ax, gdt.data 
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

.check_cpuid:
    ; check that CPUID is supported
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    xor eax, ecx
    jz .no_cpuid

    ; check if long (64-bit) mode is supported
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jl .no_long_mode

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode

.setup_pae_paging:
    ; disable old paging
;    mov eax, cr0
;    and eax, 01111111111111111111111111111111b
;    mov cr0, eax

    mov edi, 0x1000    ; Set the destination index to 0x1000.
    mov cr3, edi       ; Set control register 3 to the destination index.
    xor eax, eax       ; Nullify the A-register.
    mov ecx, 4096      ; Set the C-register to 4096.
    rep stosd          ; Clear the memory.
    mov edi, cr3       ; Set the destination index to control register 3.
    ; clear PAE paging tables
;    mov edi, 0x1000 ; PML4T -> 0x1000
;    mov cr3, edi
;    xor eax, eax
;    mov ecx, 4096
;    std             ; makes sure to decrement ecx
;    rep stosd
;    mov edi, cr3
   
    ; setup page tables
    mov dword [edi], 0x2003 ; PDPT -> 0x2000
    add edi, 0x1000
    mov dword [edi], 0x3003 ; PDT -> 0x3000
    add edi, 0x1000
    mov dword [edi], 0x4003 ; PT -> 0x4000
    add edi, 0x1000
    ; NOTE(alexander): the 3 means that page is present and readable

    mov ebx, 0x00000003          ; Set the B-register to 0x00000003.
    mov ecx, 512                 ; Set the C-register to 512.
 
.SetEntry:
    mov DWORD [edi], ebx         ; Set the uint32_t at the destination index to the B-register.
    add ebx, 0x1000              ; Add 0x1000 to the B-register.
    add edi, 8                   ; Add eight to the destination index.
    loop .SetEntry       

;    mov ebx, 0x00000003
;    mov ecx, 512
;.set_page_entry:
;    mov dword [edi], ebx
;    add ebx, 0x1000
;    add edi, 8
;    loop .set_page_entry
    
    ; enable PAE paging
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

.set_pml5_paging:
    mov eax, 0x7
    xor ecx, ecx
    cpuid
    test ecx, (1<<16)
    jz .skip_5_level_paging
    mov eax, cr4
    or eax, (1<<12) ;CR4.LA57
    mov cr4, eax
.skip_5_level_paging:
    
; TODO(alexander): investigate level-5 paging here PML5

.switch_to_long_mode:
    ; set LM-bit
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; edit the gdt to support 64-bit paging
    call edit_gdt_to_64_bit

    ; jump to kernel in 64-bit mode
    lgdt [gdt.pointer]
    jmp gdt.code:main64

.kernel32_success:
    ; kernel was initialized sucessfully
    mov esi, msg_kernel_success
    call puts
    jmp halt

.no_cpuid:
    ; no CPUID support
    mov si, msg_no_cpuid
    call puts
    jmp halt

.no_long_mode:
    ; no long mode support
    mov si, msg_no_long_mode
    call puts
    jmp halt

; 
; Prints null-terminated string from pointer.
; Parameters:
;     - ds:si: points to null-terminated string
;
puts:
    push esi
    push ebx
    push eax
    mov ebx, [buffer_display]
.loop:
    lodsb
    or al, al
    jz .exit
    or ax, 0x0300 ; TODO(alexander): color blue parameterize maybe?

    mov word [ebx], ax
    add ebx, 2
    jmp .loop
.exit:
    mov [buffer_display], ebx ; TODO(alexander): check buffer overflow
    pop eax
    pop ebx
    pop esi
    ret

halt:
    cli
    hlt
