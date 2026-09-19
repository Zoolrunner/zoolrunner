FROM oraclelinux:8@sha256:21916d0f9527aa5d0b84034dcb3f6d2f01b59e633e31221b49855032ce41069a
RUN dnf -y install \
    gcc-toolset-14-gcc gcc-toolset-14-gcc-c++ gcc-toolset-14-binutils \
    gcc-toolset-14-libstdc++-devel.x86_64 gcc-toolset-14-libstdc++-devel.i686 glibc-devel.i686 \
    make autoconf automake perl python3 flex bison pkgconf-pkg-config \
    glib2-devel.x86_64 glib2-devel.i686 gtk2-devel.x86_64 gtk2-devel.i686 \
    libXt-devel.x86_64 libXt-devel.i686 \
    xorg-x11-server-Xvfb xorg-x11-fonts-misc dejavu-sans-fonts \
    rsync tar gzip bzip2 zip unzip file which diffutils findutils patch \
    && dnf clean all
# RPM devel dependencies may be satisfied by the other multilib architecture.
# Install the target development symlinks and pkg-config files explicitly.
RUN dnf -y install \
    atk-devel.i686 cairo-devel.i686 pango-devel.i686 gdk-pixbuf2-devel.i686 \
    fontconfig-devel.i686 freetype-devel.i686 harfbuzz-devel.i686 \
    graphite2-devel.i686 fribidi-devel.i686 libicu-devel.i686 \
    libpng-devel.i686 pixman-devel.i686 zlib-devel.i686 bzip2-devel.i686 \
    expat-devel.i686 libuuid-devel.i686 libxcb-devel.i686 pcre-devel.i686 \
    libX11-devel.i686 libXau-devel.i686 libXext-devel.i686 \
    libXrender-devel.i686 libXft-devel.i686 libXfixes-devel.i686 \
    libXcomposite-devel.i686 libXcursor-devel.i686 libXinerama-devel.i686 \
    libXi-devel.i686 libXrandr-devel.i686 libICE-devel.i686 libSM-devel.i686 \
    && dnf clean all
RUN PKG_CONFIG_LIBDIR=/usr/lib/pkgconfig:/usr/share/pkgconfig pkg-config --cflags --libs gtk+-2.0 \
    && PKG_CONFIG_LIBDIR=/usr/lib64/pkgconfig:/usr/share/pkgconfig pkg-config --cflags --libs gtk+-2.0
ENV PATH=/opt/rh/gcc-toolset-14/root/usr/bin:$PATH
WORKDIR /source
RUN dnf -y install python3-pyyaml && dnf clean all
