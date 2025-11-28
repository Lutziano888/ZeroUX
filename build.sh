#!/bin/bash
set -e

echo "===== Building zeroUX ====="

rm -rf build
mkdir -p build/iso/boot/grub

# Multiboot header + boot + port asm
nasm -f elf32 multiboot_header.asm -o build/multiboot_header.o
nasm -f elf32 boot.asm -o build/boot.o
nasm -f elf32 port.asm -o build/port.o

# C modules (32-bit)
gcc -m32 -ffreestanding -c kernel.c -o build/kernel.o
gcc -m32 -ffreestanding -c vga.c -o build/vga.o
gcc -m32 -ffreestanding -c input.c -o build/input.o
gcc -m32 -ffreestanding -c string.c -o build/string.o
gcc -m32 -ffreestanding -c shell.c -o build/shell.o
gcc -m32 -ffreestanding -c rtc.c -o build/rtc.o
gcc -m32 -ffreestanding -c keyboard.c -o build/keyboard.o
gcc -m32 -ffreestanding -c calc.c -o build/calc.o
gcc -m32 -ffreestanding -c fs.c -o build/fs.o

# Link (multiboot header must come first)
ld -m elf_i386 -T linker.ld -o build/kernel.bin \
    build/multiboot_header.o \
    build/boot.o \
    build/port.o \
    build/kernel.o \
    build/vga.o \
    build/input.o \
    build/string.o \
    build/shell.o \
    build/rtc.o \
    build/keyboard.o \
    build/calc.o \
    build/fs.o

# Copy to iso tree
cp build/kernel.bin build/iso/boot/kernel.bin

# grub config
cat > build/iso/boot/grub/grub.cfg << EOF
set timeout=0
set default=0
menuentry "zeroUX" {
    multiboot /boot/kernel.bin
    boot
}
EOF

# create iso
grub-mkrescue -o zeroUX.iso build/iso

echo "===== Build complete: zeroUX.iso ====="