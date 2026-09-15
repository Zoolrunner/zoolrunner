# Build native-host QEMU separately from the 32-bit cross compiler.
# QEMU 7.2 boots console tests but fails the original 10.0 desktop startup.
FROM debian:bookworm-slim@sha256:88200866dfff7ea7f5cbcb6ec7c8a701889efe6fe859fe64d6990e4b07ea4171 AS emulator
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential pkg-config libglib2.0-dev libpixman-1-dev libfdt-dev \
    zlib1g-dev ninja-build python3-venv curl ca-certificates \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /build
RUN curl --fail --location --retry 3 \
      https://download.qemu.org/qemu-11.1.1.tar.xz -o qemu.tar.xz \
    && echo '079ffbff8a7111bbc89022107cbabf3bbfd614d5fc9d7cc675991196aca12482  qemu.tar.xz' | sha256sum -c - \
    && mkdir source obj \
    && tar -xJf qemu.tar.xz -C source --strip-components=1 \
    && cd obj \
    && ../source/configure --target-list=ppc-softmmu --disable-docs --disable-werror \
    && make -j3

FROM debian:bookworm-slim@sha256:88200866dfff7ea7f5cbcb6ec7c8a701889efe6fe859fe64d6990e4b07ea4171
RUN apt-get update && apt-get install -y --no-install-recommends \
    libglib2.0-0 libpixman-1-0 libfdt1 zlib1g hfsutils python3 ca-certificates curl \
    && rm -rf /var/lib/apt/lists/*
COPY --from=emulator /build/obj/qemu-system-ppc /usr/local/bin/qemu-system-ppc
COPY --from=emulator /build/source/pc-bios/ /usr/local/share/qemu/
WORKDIR /work
