#!/usr/bin/env bash
#   Copyright (C) 2025 John Törnblom
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

VER="20240919"
URL="https://github.com/AlisterT/openjazz/archive/refs/tags/$VER.tar.gz"
SHAREWARE="https://www.classicdosgames.com/files/games/epic/1jazz13.zip"

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"

if [[ -z "$PS5_PAYLOAD_SDK" ]]; then
    echo "error: PS5_PAYLOAD_SDK is not set"
    exit 1
fi

source "${PS5_PAYLOAD_SDK}/toolchain/prospero.sh" || exit 1

TEMPDIR=$(mktemp -d)
trap 'rm -rf -- "$TEMPDIR"' EXIT

wget -O $TEMPDIR/openjazz.tar.xz "${URL}" || exit 1
tar xf $TEMPDIR/openjazz.tar.xz -C $TEMPDIR || exit 1

${CMAKE} -DCMAKE_BUILD_TYPE=Release \
	 -B $TEMPDIR/build \
	 -S $TEMPDIR/openjazz-$VER || exit 1
${MAKE} -C $TEMPDIR/build || exit 1

mkdir -p "${SCRIPT_DIR}/sce_sys" || exit 1
mv $TEMPDIR/openjazz-$VER/doc/OpenJazz.png "${SCRIPT_DIR}/sce_sys/icon0.png" || exit 1
mv $TEMPDIR/build/OpenJazz "${SCRIPT_DIR}/openjazz.elf" || exit 1

mkdir $TEMPDIR/shareware || exit 1
cd $TEMPDIR/shareware || exit 1
wget -O shareware.zip "${SHAREWARE}" || exit 1
unzip shareware.zip || exit 1
7z x *.EXE

rm -f shareware.zip
rm -f *.EXE
mv * "${SCRIPT_DIR}/" || exit 1
