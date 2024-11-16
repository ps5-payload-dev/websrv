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

VER="ps5"
URL="https://github.com/ps5-payload-dev/yquake2/archive/refs/heads/$VER.tar.gz"
DEMO="https://ftp.gwdg.de/pub/misc/ftp.idsoftware.com/idstuff/quake2/q2-314-demo-x86.exe"
MODELS="https://deponie.yamagi.org/quake2/assets/texturepack/models.zip"
TEXTURES="https://deponie.yamagi.org/quake2/assets/texturepack/textures.zip"

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"

if [[ -z "$PS5_PAYLOAD_SDK" ]]; then
    echo "error: PS5_PAYLOAD_SDK is not set"
    exit 1
fi

source "${PS5_PAYLOAD_SDK}/toolchain/prospero.sh" || exit 1

TEMPDIR=$(mktemp -d)
trap 'rm -rf -- "$TEMPDIR"' EXIT

wget -O "${SCRIPT_DIR}/baseq2/models.zip" "${MODELS}" || exit 1
wget -O "${SCRIPT_DIR}/baseq2/textures.zip" "${TEXTURES}" || exit 1

wget -O $TEMPDIR/yquake2.tar.gz "${URL}" || exit 1
wget -O $TEMPDIR/install.exe "${DEMO}" || exit 1

tar xf $TEMPDIR/yquake2.tar.gz -C $TEMPDIR || exit 1
7z x -o$TEMPDIR $TEMPDIR/install.exe || exit 1

sed -i 's|SDL2_LIBRARY|SDL2_LIBRARIES|g' $TEMPDIR/yquake2-$VER/CMakeLists.txt

cd $TEMPDIR

${CMAKE} -DCMAKE_BUILD_TYPE=Release \
	 -DGL1_RENDERER=NO \
	 -DGL3_RENDERER=NO \
	 -DGLES1_RENDERER=NO \
	 -DGLES3_RENDERER=NO \
	 -B $TEMPDIR/build \
         -S $TEMPDIR/yquake2-$VER
${CMAKE} --build $TEMPDIR/build

mv $TEMPDIR/build/release/quake2 "${SCRIPT_DIR}/quake2.elf" || exit 1
mv $TEMPDIR/build/release/ref_soft.so "${SCRIPT_DIR}/" || exit 1
mv $TEMPDIR/build/release/baseq2/game.so "${SCRIPT_DIR}/baseq2/" || exit 1
mv $TEMPDIR/Install/Data/baseq2/pak0.pak "${SCRIPT_DIR}/baseq2/" || exit 1
mv $TEMPDIR/Install/Data/baseq2/players "${SCRIPT_DIR}/baseq2/" || exit 1
