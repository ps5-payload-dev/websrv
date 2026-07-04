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

VER="sdl2remap"
URL="https://github.com/lazd/wolf4sdl/archive/refs/heads/$VER.tar.gz"
ASSETS="https://beta.wolf3d.net/file-download/download/private/4457"

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

wget -O $TEMPDIR/wolf4sdl.tar.gz "${URL}"
wget -O $TEMPDIR/shareware.zip "${ASSETS}"

cd $TEMPDIR
tar xf wolf4sdl.tar.gz

export SDL_CONFIG="${PS5_PAYLOAD_SDK}"/bin/prospero-sdl2-config
export CFLAGS="-Wno-c++11-narrowing"
export DATADIR="./"

# build retail (v1.4)
${MAKE} -C "$TEMPDIR/wolf4sdl-sdl2remap"
mv "${TEMPDIR}/wolf4sdl-sdl2remap/wolf3d" "${SCRIPT_DIR}/wolf3d.elf"

# build shareware (v1.4)
sed -i 's|#define GOODTIMES|//#define GOODTIMES|' "${TEMPDIR}/wolf4sdl-sdl2remap/version.h"
export CFLAGS="-Wno-c++11-narrowing -DUPLOAD"
${MAKE} -C "$TEMPDIR/wolf4sdl-sdl2remap" clean all

mv "${TEMPDIR}/wolf4sdl-sdl2remap/wolf3d" "${SCRIPT_DIR}/wolf3d-shareware.elf"

# copy shareware assets
cd $TEMPDIR
unzip shareware.zip
for f in *.WL1; do
    mv -- "$f" "${SCRIPT_DIR}/${f,,}"
done
