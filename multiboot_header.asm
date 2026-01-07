; multiboot_header.asm
BITS 32
section .multiboot
align 4
MB_MAGIC       equ 0x1BADB002
; Flags: bit 2 = request graphics info
MB_FLAGS       equ 0x00000004
MB_CHECKSUM    equ -(MB_MAGIC + MB_FLAGS)
dd MB_MAGIC
dd MB_FLAGS
dd MB_CHECKSUM
; Graphics mode info (when bit 2 is set)
dd 0            ; graphics mode (0 = linear framebuffer mode)
dd 1024         ; width
dd 768          ; height
dd 32           ; depth (bits per pixel)
