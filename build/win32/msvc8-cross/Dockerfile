# Wine 11 WoW64 runs the x86 compiler using an amd64 Linux host.
FROM debian:bookworm-slim@sha256:5ae3c39ebd15e229dcedd5cee596b2497182493d41ff162e824ba13fc1b2b867
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential autoconf automake libglib2.0-dev pkg-config python3 perl \
    zip unzip bzip2 flex bison curl ca-certificates rsync file binutils \
    p7zip-full cabextract xvfb xauth \
    libasound2 libglib2.0-0 libgphoto2-6 libgphoto2-port12 \
    libgstreamer-plugins-base1.0-0 libgstreamer1.0-0 libpcap0.8 libpulse0 \
    libsane1 libudev1 libunwind8 libusb-1.0-0 libwayland-client0 libwayland-egl1 \
    libx11-6 libxext6 libxkbcommon0 libxkbregistry0 ocl-icd-libopencl1 \
    libasound2-plugins libncurses6 libfreetype6 libfontconfig1 libgnutls30 \
    libxrender1 libxcursor1 libxi6 libxrandr2 fonts-liberation \
    && rm -rf /var/lib/apt/lists/*
COPY fetch-wine.sh /tmp/fetch-wine.sh
RUN sh /tmp/fetch-wine.sh && rm /tmp/fetch-wine.sh
COPY fetch-toolchain.sh /tmp/fetch-toolchain.sh
RUN sh /tmp/fetch-toolchain.sh /opt/msvc8 && rm /tmp/fetch-toolchain.sh
ENV PATH=/opt/wine-stable/bin:$PATH
ENV WINEARCH=wow64 WINEPREFIX=/work/wine WINEDEBUG=-all \
    WINEDLLOVERRIDES=mscoree,mshtml= MSVC8_ROOT=/opt/msvc8
WORKDIR /source
