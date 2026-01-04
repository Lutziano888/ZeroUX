#!/bin/bash

echo "===== Building zeroUX with GUI ====="

# Erstelle build/apps Ordner
mkdir -p build/apps

# Kompiliere alle C-Dateien
gcc -m32 -c kernel.c -o build/kernel.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c vga.c -o build/vga.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c keyboard.c -o build/keyboard.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c string.c -o build/string.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c shell.c -o build/shell.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c calc.c -o build/calc.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c rtc.c -o build/rtc.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c fs.c -o build/fs.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c input.c -o build/input.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c mouse.c -o build/mouse.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c gui.c -o build/gui.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c widgets.c -o build/widgets.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1

# Kompiliere Apps
echo "===== Compiling Apps ====="
gcc -m32 -c apps/calculator.c -o build/apps/calculator.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c apps/notepad.c -o build/apps/notepad.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1
gcc -m32 -c apps/welcome.c -o build/apps/welcome.o -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -I. || exit 1

# Link alles zusammen
echo "===== Linking ====="
ld -m elf_i386 -T linker.ld -o build/kernel.elf \
    build/kernel.o \
    build/vga.o \
    build/keyboard.o \
    build/string.o \
    build/shell.o \
    build/calc.o \
    build/rtc.o \
    build/fs.o \
    build/input.o \
    build/mouse.o \
    build/gui.o \
    build/widgets.o \
    build/apps/calculator.o \
    build/apps/notepad.o \
    build/apps/welcome.o

if [ $? -ne 0 ]; then
    echo "FEHLER: Linking fehlgeschlagen!"
    exit 1
fi

# Erstelle ISO
echo "===== Creating ISO ====="
mkdir -p iso/boot/grub
cp build/kernel.elf iso/boot/kernel.elf

# Erstelle grub.cfg wenn sie nicht existiert oder kopiere sie
if [ -f grub.cfg ]; then
    cp grub.cfg iso/boot/grub/grub.cfg
else
    cat > iso/boot/grub/grub.cfg << 'EOF'
set timeout=3
set default=0

menuentry "zeroUX GUI" {
    multiboot /boot/kernel.elf
    boot
}
EOF
fi

grub-mkrescue -o zeroUX.iso iso

if [ $? -ne 0 ]; then
    echo "FEHLER: Keine ISO erzeugt!"
    exit 1
fi

echo "===== Build erfolgreich! ====="
echo "ISO erstellt: zeroUX.iso"