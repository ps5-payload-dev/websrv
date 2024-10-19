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

VER="1.5.2"
URL="https://github.com/diasurgical/devilutionX/releases/download/$VER/devilutionx-src.tar.xz"
MPQ="https://github.com/diasurgical/devilutionx-assets/releases/download/v2/spawn.mpq"

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"

if [[ -z "$PS5_PAYLOAD_SDK" ]]; then
    echo "error: PS5_PAYLOAD_SDK is not set"
    exit 1
fi

source "${PS5_PAYLOAD_SDK}/toolchain/prospero.sh" || exit 1

TEMPDIR=$(mktemp -d)
trap 'rm -rf -- "$TEMPDIR"' EXIT

wget -O $TEMPDIR/devilutionx-src.tar.xz "${URL}" || exit 1
tar xf $TEMPDIR/devilutionx-src.tar.xz -C $TEMPDIR || exit 1

${CMAKE} -DCMAKE_BUILD_TYPE=Release \
	 -DDISCORD_INTEGRATION=OFF \
	 -DBUILD_TESTING=OFF \
	 -DASAN=OFF \
	 -DUBSAN=OFF \
	 -DDISABLE_LTO=ON \
	 -DNOEXIT=ON \
	 -DNONET=OFF \
	 -DBUILD_ASSETS_MPQ=ON \
	 -DDEVILUTIONX_SYSTEM_SDL_IMAGE=OFF \
	 -B $TEMPDIR/build \
	 -S $TEMPDIR/devilutionx-src-$VER || exit 1
${MAKE} -C $TEMPDIR/build || exit 1

wget -O "${SCRIPT_DIR}/spawn.mpq" "${MPQ}" || exit 1
mv $TEMPDIR/build/devilutionx "${SCRIPT_DIR}/devilutionx.elf" || exit 1
mv $TEMPDIR/build/devilutionx.mpq "${SCRIPT_DIR}/devilutionx.mpq" || exit 1
