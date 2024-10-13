#!/bin/bash

# Get precompiled libraries so we don't have to brew-compile them
wget https://github.com/tahoma2d/qt5/releases/download/v5.15.2/Qt5.15.10_macOS-12.tar.gz
tar -zxf Qt5.15.10_macOS-12.tar.gz
mv qt@5 /usr/local/Cellar
ln -s  /usr/local/Cellar/qt@5/5.15.10_1 /usr/local/opt/qt@5

wget https://github.com/tahoma2d/boost/releases/download/boost-Tahoma2D/boost1.86.0_macOS-12.tar.gz
tar -zxf boost1.86.0_macOS-12.tar.gz
mv boost /usr/local/Cellar
ln -s /usr/local/Cellar/boost /usr/local/opt/boost

# Remove symlink to bin/2to3 in order for latest python to install
rm -f /usr/local/bin/2to3-3.12
rm -f /usr/local/bin/idle3
rm -f /usr/local/bin/idle3.12
rm -f /usr/local/bin/pydoc3
rm -f /usr/local/bin/pydoc3.12

brew update
# Remove synlink to nghttp2 in order for latest curl to install
#brew unlink nghttp2
# Removed boost qt@5 because it's keg only and takes hours to compile.
#brew install boost qt@5
brew install clang-format glew lz4 lzo libmypaint jpeg-turbo nasm yasm fontconfig freetype gnutls lame libass libbluray libsoxr libvorbis libvpx opencore-amr openh264 openjpeg opus rav1e sdl2 snappy speex tesseract theora webp xvid xz gsed
#brew install dav1d
brew install meson ninja
brew install automake autoconf gettext pkg-config libtool libusb-compat gd libexif
