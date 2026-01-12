; Dateiname: interrupts.asm
; Dieses File muss mit NASM assembliert werden (z.B. nasm -f elf32 interrupts.asm -o interrupts.o)

extern mouse_callback ; Unsere C-Funktion in interrupts.c

global irq12      ; Exportieren, damit idt.c sie findet
global idt_load   ; Exportieren, damit idt.c sie findet

section .text

; Interrupt Service Routine für IRQ 12 (Maus)
irq12:
    cli          ; Interrupts sperren, um Verschachtelung zu verhindern
    pusha        ; Alle allgemeinen Register sichern (eax, ecx, edx, ebx, esp, ebp, esi, edi)

    call mouse_callback ; Unsere C-Funktion aufrufen

    ; "End of Interrupt" (EOI) an die PICs senden
    ; IRQ 12 kommt vom Slave-PIC, also müssen beide informiert werden.
    mov al, 0x20
    out 0xA0, al ; EOI an Slave-PIC
    out 0x20, al ; EOI an Master-PIC

    popa         ; Alle Register wiederherstellen
    sti          ; Interrupts wieder global aktivieren
    iret         ; Rückkehr aus dem Interrupt

; Lädt die IDT-Struktur in das IDTR-Register der CPU
idt_load:
    mov eax, [esp+4] ; Adresse des idt_ptr vom Stack holen
    lidt [eax]       ; IDT laden
    ret
