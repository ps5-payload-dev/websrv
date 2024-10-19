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

VER="20240725-10593-19c21b9ab"
URL="http://dukeworld.duke4.net/eduke32/synthesis/$VER/eduke32_src_$VER.tar.xz"
DEMO="http://dukeworld.duke4.net/classic%20dukeworld/share/3dduke13.zip"

SCRIPT_PATH="${BASH_SOURCE[0]}"
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"

if [[ -z "$PS5_PAYLOAD_SDK" ]]; then
    echo "error: PS5_PAYLOAD_SDK is not set"
    exit 1
fi

source "${PS5_PAYLOAD_SDK}/toolchain/prospero.sh" || exit 1

TEMPDIR=$(mktemp -d)
trap 'rm -rf -- "$TEMPDIR"' EXIT

wget -O $TEMPDIR/demo.zip "${DEMO}" || exit 1
cd $TEMPDIR
unzip demo.zip || exit 1
unzip -Luq "DN3DSW13.SHR" defs.con duke.rts game.con license.txt user.con duke3d.grp
mv defs.con duke.rts game.con license.txt user.con duke3d.grp "${SCRIPT_DIR}/"


wget -O $TEMPDIR/eduke32.tar.xz "${URL}" || exit 1
tar xf $TEMPDIR/eduke32.tar.xz -C $TEMPDIR || exit 1
cd $TEMPDIR/eduke32_$VER || exit 1

sed -i 's|#include <malloc.h>|#include <stdlib.h>|g' source/build/include/compat.h
sed -i 's|-fuse-ld=lld||g' Common.mak
sed -i 's|-lexecinfo||g' GNUmakefile
sed -i 's|define PRINTSTACKONSEGV 1||g' source/build/src/sdlayer.cpp
sed -i 's|pthread_get_name_np|//|g' source/build/src/loguru.cpp

CFLAGS="$($PS5_PAYLOAD_SDK/bin/prospero-sdl2-config --cflags)"
LDFLAGS="$($PS5_PAYLOAD_SDK/bin/prospero-sdl2-config --libs)"
${MAKE} PRETTY_OUTPUT=0 USE_OPENGL=0 HAVE_GTK2=0 \
	USE_MIMALLOC=0 PLATFORM=BSD LTO=0 \
	SDLCONFIG="${PS5_PAYLOAD_SDK}/bin/prospero-sdl2-config" \
	CC="$CC" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" || exit 1

mv eduke32 "${SCRIPT_DIR}/eduke32.elf" || exit 1

