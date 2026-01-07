@echo off
set DOCKER_BUILDKIT=0

echo ========= zeroUX Build =========

docker build -t zeroux-builder .
docker run --rm -v "%cd%":/zeroux zeroux-builder

if exist zeroUX.iso (
    echo ISO erfolgreich erstellt!
) else (
    echo FEHLER: Keine ISO erzeugt!
)

pause