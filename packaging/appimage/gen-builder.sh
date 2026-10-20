#! /bin/sh

while getopts a:d:p:u: arg
do
    case $arg in
        a) arch="$OPTARG" ;;
        d) dir="$OPTARG" ;;
        p) dpkgarch="$OPTARG" ;;
        u) url="$OPTARG" ;;
        ?) printf "%s [-d <dir>] [-a <arch>] [-p <dpkgarch>] [-u <url>]\n" "$0"
    esac
done

dir=${dir:-"/AppDir"}
arch=${arch:-"$(uname -m)"}
dpkgarch=${dpkgarch:-"$(dpkg --print-architecture)"}
url=${url:-"http://archive.ubuntu.com/ubuntu"}

cat <<EOF
version: 1
AppDir:
  path: $dir
  app_info:
    id: net.speed_dreams.SpeedDreams
    name: speed-dreams
    icon: icon
    version: $(git describe --tags --dirty)
    exec: usr/games/speed-dreams-2
    exec_args: \$@
  after_runtime: |
    cd "\$TARGET_APPDIR/lib/$arch-linux-gnu/"
    while read f
    do
      ln -rs ../../runtime/compat/lib/$arch-linux-gnu/\$f \$f
    done <<EOF
    libcrypt.so.1
    libcrypt.so.1.1.0
    libdl-2.31.so
    libdl.so.2
    libnsl-2.31.so
    libnsl.so.1
    libresolv-2.31.so
    libresolv.so.2
    libz.so.1
    libz.so.1.2.11
    EOF
  apt:
    arch:
    - $dpkgarch
    allow_unauthenticated: true
    sources:
    - sourceline: deb [signed-by="/usr/share/keyrings/ubuntu-archive-keyring.gpg"]
        $url focal main universe
    - sourceline: deb [signed-by="/usr/share/keyrings/ubuntu-archive-keyring.gpg"]
        $url focal-updates main universe
    - sourceline: deb [signed-by="/usr/share/keyrings/ubuntu-archive-keyring.gpg"]
        $url focal-security main universe
    include:
    - libc6:$dpkgarch
    - locales
    - libcrypt1
    - libenet7
    - libexpat1
    - libglu1-mesa
    - libglx0
    - libgl1
    - libglvnd0
    - libopengl0
    - libopenscenegraph160
    - libcurl4
    - libsdl2-2.0-0
    - libsdl2-mixer-2.0-0
    - libsdl2-ttf-2.0-0
    - librhash0
    - libpng16-16
    - libjpeg8
    - libx11-6
    - libwayland-egl1
    - libwayland-client0
    - libwayland-cursor0
    - libxau6
    - libxdmcp6
    - libxcb1
    - zlib1g
    - libminizip1
    - libopenal1
    - libplib1
    - bash
    - dash
    - perl
  files:
    include:
    - lib64/ld-linux-$(echo $arch | tr _ -).so.2
    - usr/share/games/speed-dreams-2/*
    - usr/lib/games/speed-dreams-2/*
    - /usr/local/lib/libcjson.so*
    exclude:
    - usr/share/man
    - usr/share/doc/*/README.*
    - usr/share/doc/*/changelog.*
    - usr/share/doc/*/NEWS.*
    - usr/share/doc/*/TODO.*
AppImage:
  arch: $arch
  update-information: guess
EOF
