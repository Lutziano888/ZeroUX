FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    nasm \
    gcc \
    gcc-multilib \
    libc6-dev-i386 \
    binutils \
    grub-pc-bin \
    grub-common \
    xorriso \
    mtools \
    make \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /zeroux

COPY . .

CMD ["bash", "build.sh"]
