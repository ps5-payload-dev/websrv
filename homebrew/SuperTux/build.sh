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

VER="v0.7.0-rev.1"
URL="https://github.com/SuperTux/supertux.git"

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

git clone --recurse-submodules --shallow-submodules --depth 1 --branch $VER $URL "$TEMPDIR/supertux"

export SDL_CONFIG="${PS5_PAYLOAD_SDK}/bin/prospero-sdl2-config"

sed -i 's|SDL2_image::SDL2_image|SDL2_image::SDL2_image-static|g' $TEMPDIR/supertux/CMakeLists.txt
sed -i 's|-lexecinfo||g' $TEMPDIR/supertux/CMakeLists.txt

${CMAKE} -DCMAKE_BUILD_TYPE=Release \
     -DBUILD_DOCUMENTATION=OFF \
     -DENABLE_DISCORD=OFF \
     -DUSE_STATIC_SIMPLESQUIRREL=ON \
     -DUSE_SYSTEM_SDL2_TTF=ON \
     -B $TEMPDIR/build \
     $TEMPDIR/supertux
${MAKE} -C $TEMPDIR/build

mkdir -p "${SCRIPT_DIR}/sce_sys"
cp $TEMPDIR/supertux/data/images/engine/icons/supertux-256x256.png "${SCRIPT_DIR}/sce_sys/icon0.png"

mv $TEMPDIR/build/supertux2 "${SCRIPT_DIR}/supertux2.elf"
cp "${PS5_SYSROOT}/${PS5_HBROOT}/lib/libOSMesa.so.8" "${SCRIPT_DIR}/libOSMesa.so.8"

mv $TEMPDIR/supertux/data/ "${SCRIPT_DIR}/"
mv $TEMPDIR/supertux/LICENSE.txt "${SCRIPT_DIR}/"
mv $TEMPDIR/supertux/NEWS.md "${SCRIPT_DIR}/"
mv $TEMPDIR/supertux/README.md "${SCRIPT_DIR}/"