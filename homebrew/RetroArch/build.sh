#!/usr/bin/env bash
#   Copyright (C) 2024 John Törnblom
#
# This file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING. If not see
# <http://www.gnu.org/licenses/>.

VER="1.19.1"
URL="https://github.com/libretro/RetroArch/archive/refs/tags/v$VER.tar.gz"

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"

if [[ -z "$PS5_PAYLOAD_SDK" ]]; then
    echo "error: PS5_PAYLOAD_SDK is not set"
    exit 1
fi

source "${PS5_PAYLOAD_SDK}/toolchain/prospero.sh" || exit 1

TEMPDIR=$(mktemp -d)
trap 'rm -rf -- "$TEMPDIR"' EXIT

wget -O $TEMPDIR/RetroArch.tar.gz "${URL}" || exit 1
tar xf  $TEMPDIR/RetroArch.tar.gz -C $TEMPDIR || exit 1

cd $TEMPDIR/RetroArch-$VER || exit 1
sed -i 's|SDL_RENDERER_ACCELERATED|SDL_RENDERER_SOFTWARE|g' gfx/drivers/sdl2_gfx.c

export CC
export CXX
export PKG_CONFIG
export CROSS_COMPILE="${PS5_PAYLOAD_SDK}/bin/prospero-"
export OS="BSD"
export DISTRO=
export CFLAGS="-O1"
./configure \
    --prefix="${PREFIX}" \
    --enable-sdl2 \
    --enable-ffmpeg \
    --enable-dylib \
    --enable-networking \
    --enable-zlib \
    --enable-freetype \
    --enable-sse \
    --enable-flac \
    --disable-builtinflac \
    --disable-builtinmbedtls \
    --disable-builtinbearssl \
    --disable-builtinzlib \
    --disable-builtinglslang \
    --disable-crtswitchres \
    --disable-discord \
    --disable-qt \
    --disable-ssa \
    --disable-jack \
    --disable-oss \
    --disable-sdl \
    --disable-cg \
    --disable-vulkan \
    --disable-opengl_core \
    --disable-microphone \
    --enable-debug
${MAKE} V=1 || exit 1

mkdir -p "${SCRIPT_DIR}/sce_sys"
mv $TEMPDIR/RetroArch-$VER/media/icons/playstore/icon.png "${SCRIPT_DIR}/sce_sys/icon0.png" || exit 1
mv $TEMPDIR/RetroArch-$VER/retroarch.cfg "${SCRIPT_DIR}/retroarch.cfg"
mv $TEMPDIR/RetroArch-$VER/retroarch "${SCRIPT_DIR}/retroarch.elf" || exit 1
