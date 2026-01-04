bits 32

section .multiboot
    dd 0x1BADB002              ; Magic number
    dd 0x00                    ; Flags
    dd -(0x1BADB002 + 0x00)   ; Checksum

section .text
global _start
extern kernel_main

_start:
    cli
    mov esp, stack_top
    
    call kernel_main
    
    cli
.hang:
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 16384  ; 16 KB Stack
stack_top: