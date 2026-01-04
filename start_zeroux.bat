@echo off
echo Starte ZeroUX...

set QEMU="C:\Program Files\qemu\qemu-system-x86_64.exe"
set QEMU_IMG="C:\Program Files\qemu\qemu-img.exe"

set ISO="D:\ZMC\ZeroOs\zeroUX.iso"
set DISK="D:\ZMC\ZeroOs\zeroux_disk.qcow2"

REM Disk anlegen, falls nicht vorhanden
if not exist %DISK% (
    echo Erstelle virtuelle Festplatte...
    %QEMU_IMG% create -f qcow2 %DISK% 10G
)

echo QEMU gestartet...
%QEMU% ^
    -accel whpx ^
    -cpu qemu32 ^
    -m 512 ^
    -smp 1 ^
    -hda %DISK% ^
    -cdrom %ISO% ^
    -boot d ^
    -vga std

pause