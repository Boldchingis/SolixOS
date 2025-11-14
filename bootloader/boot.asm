; SolixOS Bootloader
; Loads kernel at 0x100000 and transfers control
; MBR format, 512 bytes, bootable flag at 0x7C00

[BITS 16]
[ORG 0x7C00]

start:
    ; Clear screen
    mov ax, 0x0003
    int 0x10
    
    ; Display boot message
    mov si, boot_msg
    call print_string
    
    ; Reset disk system
    mov ax, 0x0000
    int 0x13
    jc disk_error
    
    ; Load kernel from disk (sectors 2-33)
    mov ax, 0x1000    ; ES:BX = 0x1000:0x0000 (64KB)
    mov es, ax
    xor bx, bx
    
    mov ah, 0x02      ; Read sectors
    mov al, 32        ; Number of sectors to read (16KB)
    mov ch, 0         ; Cylinder 0
    mov dh, 0         ; Head 0
    mov cl, 2         ; Sector 2 (skip bootloader)
    int 0x13
    jc disk_error
    
    ; Verify kernel loaded
    cmp al, 32
    jne disk_error
    
    ; Display success message
    mov si, success_msg
    call print_string
    
    ; Disable interrupts and prepare for protected mode
    cli
    
    ; Load GDT
    lgdt [gdt_descriptor]
    
    ; Enable protected mode
    mov eax, cr0
    or eax, 0x00000001
    mov cr0, eax
    
    ; Far jump to protected mode code
    jmp CODE_SEG:init_pm

; 16-bit print string function
print_string:
    lodsb
    cmp al, 0
    je print_done
    mov ah, 0x0E
    int 0x10
    jmp print_string
print_done:
    ret

disk_error:
    mov si, error_msg
    call print_string
    jmp $

; Messages
boot_msg db 'SolixOS Bootloader v1.0', 13, 10, 0
success_msg db 'Kernel loaded successfully!', 13, 10, 0
error_msg db 'Disk error - System halted', 13, 10, 0

; GDT setup
gdt_start:
    dq 0  ; Null descriptor

gdt_code:
    dw 0xFFFF    ; Limit (low)
    dw 0x0000    ; Base (low)
    db 0x00      ; Base (mid)
    db 10011010b ; Access byte
    db 11001111b ; Flags + Limit (high)
    db 0x00      ; Base (high)

gdt_data:
    dw 0xFFFF    ; Limit (low)
    dw 0x0000    ; Base (low)
    db 0x00      ; Base (mid)
    db 10010010b ; Access byte
    db 11001111b ; Flags + Limit (high)
    db 0x00      ; Base (high)

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

[BITS 32]

init_pm:
    ; Set up segment registers
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Set up stack
    mov ebp, 0x90000
    mov esp, ebp
    
    ; Jump to kernel entry point
    jmp 0x10000

; Boot signature
times 510-($-$$) db 0
dw 0xAA55
