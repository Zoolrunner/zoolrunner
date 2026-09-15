# Build with: docker build --platform linux/386 -f build/macosx/powerpc.Dockerfile
#             -t zoolrunner-powerpc-toolchain build/macosx
FROM debian:bookworm-slim@sha256:88200866dfff7ea7f5cbcb6ec7c8a701889efe6fe859fe64d6990e4b07ea4171
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential autoconf libglib2.0-dev pkg-config python3 perl zip unzip \
    bzip2 flex bison curl ca-certificates xz-utils rsync libssl-dev uuid-dev patch \
    && rm -rf /var/lib/apt/lists/*
COPY fetch-ppc-toolchain.sh /tmp/fetch-ppc-toolchain.sh
RUN sh /tmp/fetch-ppc-toolchain.sh && rm /tmp/fetch-ppc-toolchain.sh
WORKDIR /source
