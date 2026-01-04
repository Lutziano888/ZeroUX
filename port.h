#ifndef PORT_H
#define PORT_H

// Byte I/O
static inline void outb(unsigned short port, unsigned char val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Word I/O
static inline void outw(unsigned short port, unsigned short val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline unsigned short inw(unsigned short port) {
    unsigned short ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Long (32-bit) I/O
static inline void outl(unsigned short port, unsigned int val) {
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline unsigned int inl(unsigned short port) {
    unsigned int ret;
    __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Alternative names for compatibility
#define port_byte_in inb
#define port_byte_out outb
#define port_word_in inw
#define port_word_out outw
#define port_long_in inl
#define port_long_out outl

#endif // PORT_H