FROM oraclelinux:8@sha256:21916d0f9527aa5d0b84034dcb3f6d2f01b59e633e31221b49855032ce41069a
# Native AArch64 tools and target libraries; no x86 multilib dependencies.
RUN test "$(uname -m)" = aarch64 && dnf -y install \
    gcc-toolset-14-gcc gcc-toolset-14-gcc-c++ gcc-toolset-14-binutils \
    gcc-toolset-14-libstdc++-devel \
    make autoconf automake perl python3 git flex bison pkgconf-pkg-config \
    glib2-devel gtk2-devel libXt-devel \
    xorg-x11-server-Xvfb xorg-x11-fonts-misc dejavu-sans-fonts \
    rsync tar gzip bzip2 zip unzip file which diffutils findutils patch \
    && dnf clean all
RUN pkg-config --cflags --libs gtk+-2.0
ENV PATH=/opt/rh/gcc-toolset-14/root/usr/bin:$PATH
WORKDIR /source
