#!/usr/bin/env bash
#   Copyright (C) 2026 John Törnblom
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

VER="v1.3.7.3"
URL="https://wohlsoft.ru/projects/TheXTech/_downloads/releases/thextech-full-src-$VER.tar.gz"
ASSETS="https://wohlsoft.ru/projects/TheXTech/_downloads/assets/thextech-smbx13-assets-full.7z"

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"

if [[ -z "$PS5_PAYLOAD_SDK" ]]; then
    echo "error: PS5_PAYLOAD_SDK is not set"
    exit 1
fi

set -e

source "${PS5_PAYLOAD_SDK}/toolchain/prospero.sh"

TEMPDIR=$(mktemp -d)
trap 'rm -rf -- "$TEMPDIR"' EXIT

wget -O $TEMPDIR/smbx13.7z "${ASSETS}"

wget -O $TEMPDIR/thextech.tar.gz "${URL}"
tar xf $TEMPDIR/thextech.tar.gz -C $TEMPDIR

export SDL_CONFIG="${PS5_PAYLOAD_SDK}/bin/prospero-sdl2-config"

${CMAKE} -DCMAKE_BUILD_TYPE=Release \
     -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
     -DPGE_ENABLE_VIDEO_REC=OFF \
     -DPGE_SHARED_SDLMIXER=OFF \
     -DTHEXTECH_BUILD_GL_DESKTOP_MODERN=OFF \
     -DTHEXTECH_BUILD_GL_ES_LEGACY=OFF \
     -DTHEXTECH_BUILD_GL_ES_MODERN=OFF \
     -DTHEXTECH_FORCE_FULLSCREEN=ON \
     -DTHEXTECH_NO_BUILD_DATE=ON \
     -DUSE_STATIC_LIBC=ON \
     -DUSE_SYSTEM_SDL2=ON \
     -B $TEMPDIR/build \
     $TEMPDIR/thextech-full-src

# must set DESTDIR or it prefixes ${PS5_PAYLOAD_SDK}/target to the path and breaks building
${MAKE} -j$(nproc) -C $TEMPDIR/build DESTDIR="/"

7z x $TEMPDIR/smbx13.7z -o$TEMPDIR/assets/

# extract icon
mkdir -p "${SCRIPT_DIR}/sce_sys"
cp $TEMPDIR/assets/graphics/ui/icon/thextech_256.png "${SCRIPT_DIR}/sce_sys/icon0.png"

mv $TEMPDIR/build/output/bin/thextech "${SCRIPT_DIR}/thextech-smbx.elf"
cp "${PS5_SYSROOT}/${PS5_HBROOT}/lib/libOSMesa.so.8" "${SCRIPT_DIR}/libOSMesa.so.8"

mv $TEMPDIR/assets/* "${SCRIPT_DIR}/"
mv $TEMPDIR/thextech-full-src/LICENSE "${SCRIPT_DIR}/License.TheXTech.txt"
mv $TEMPDIR/thextech-full-src/README.md "${SCRIPT_DIR}/ReadMe.txt"
mv $TEMPDIR/thextech-full-src/changelog.txt "${SCRIPT_DIR}/"