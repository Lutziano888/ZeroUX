BITS 32

global outb
global inb
global outw
global inw

; -------------------------
; Write 8-bit to port
; void outb(uint16_t port, uint8_t value)
; -------------------------
outb:
    mov dx, [esp + 4]     ; port
    mov al, [esp + 8]     ; value
    out dx, al
    ret

; -------------------------
; Read 8-bit from port
; uint8_t inb(uint16_t port)
; -------------------------
inb:
    mov dx, [esp + 4]     ; port
    in al, dx
    ret

; -------------------------
; Write 16-bit to port
; void outw(uint16_t port, uint16_t value)
; -------------------------
outw:
    mov dx, [esp + 4]     ; port
    mov ax, [esp + 8]     ; value
    out dx, ax
    ret

; -------------------------
; Read 16-bit from port
; uint16_t inw(uint16_t port)
; -------------------------
inw:
    mov dx, [esp + 4]     ; port
    in ax, dx
    ret
