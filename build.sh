#!/bin/bash

echo "===== Building zeroUX with VBE Graphics ====="

mkdir -p build/apps

# Compile C files
echo "Compiling core files..."
gcc -m32 -c kernel.c -o build/kernel.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c vbe.c -o build/vbe.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c font.c -o build/font.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c keyboard.c -o build/keyboard.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c string.c -o build/string.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c mouse.c -o build/mouse.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c rtc.c -o build/rtc.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c vga.c -o build/vga.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c fs.c -o build/fs.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c ethernet.c -o build/ethernet.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c gui.c -o build/gui.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1

# Compile apps
echo "Compiling apps..."
gcc -m32 -c apps/calculator.c -o build/apps/calculator.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c apps/notepad.c -o build/apps/notepad.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c apps/welcome.c -o build/apps/welcome.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c apps/filemanager.c -o build/apps/filemanager.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c apps/google_browser.c -o build/apps/google_browser.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1

# Assemble boot code
echo "Assembling boot..."
nasm -f elf32 boot.asm -o build/boot.o || exit 1

# Link everything
echo "Linking..."
ld -m elf_i386 -T linker.ld -o build/kernel.elf \
    build/boot.o \
    build/kernel.o \
    build/vbe.o \
    build/font.o \
    build/keyboard.o \
    build/string.o \
    build/mouse.o \
    build/rtc.o \
    build/vga.o \
    build/fs.o \
    build/ethernet.o \
    build/gui.o \
    build/apps/calculator.o \
    build/apps/notepad.o \
    build/apps/welcome.o \
    build/apps/filemanager.o \
    build/apps/google_browser.o || exit 1

# Create ISO
echo "Creating ISO..."
mkdir -p iso/boot/grub
cp build/kernel.elf iso/boot/kernel.elf

cat > iso/boot/grub/grub.cfg << 'EOF'
set timeout=3
set default=0

menuentry "zeroUX VBE Graphics" {
    multiboot /boot/kernel.elf
    boot
}
EOF

grub-mkrescue -o zeroUX.iso iso || exit 1

echo "===== Build successful! ====="
echo "ISO created: zeroUX.iso"
echo ""
echo "Run with QEMU:"
echo "qemu-system-i386 -cdrom zeroUX.iso -m 512 -vga std"