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

VER="2.9.0"
URL="https://downloads.scummvm.org/frs/scummvm/$VER/scummvm-$VER.tar.xz"

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"

if [[ -z "$PS5_PAYLOAD_SDK" ]]; then
    echo "error: PS5_PAYLOAD_SDK is not set"
    exit 1
fi

source "${PS5_PAYLOAD_SDK}/toolchain/prospero.sh" || exit 1

TEMPDIR=$(mktemp -d)
trap 'rm -rf -- "$TEMPDIR"' EXIT
wget -O $TEMPDIR/scummvm.tar.xz "${URL}" || exit 1
tar xf $TEMPDIR/scummvm.tar.xz -C $TEMPDIR || exit 1
cd $TEMPDIR/scummvm-$VER || exit 1

export PKG_CONFIG_DIR=
export PKG_CONFIG_SYSROOT_DIR=${PS5_PAYLOAD_SDK}/target
export PKG_CONFIG_LIBDIR=${PKG_CONFIG_SYSROOT_DIR}/user/homebrew/lib/pkgconfig
export PKG_CONFIG_PATH=${PKG_CONFIG_SYSROOT_DIR}/user/homebrew/libdata/pkgconfig

./configure --prefix="${PREFIX}" --host=x86_64-pc-freebsd \
	    --enable-static --enable-release --enable-all-engines \
	    --disable-seq-midi \
	    --with-sdl-prefix="${PS5_PAYLOAD_SDK}/target/user/homebrew/bin" \
	    --with-libcurl-prefix="${PS5_PAYLOAD_SDK}/target/user/homebrew/bin" \
	    --with-freetype2-prefix="${PS5_PAYLOAD_SDK}/target/user/homebrew/bin" \
	    --enable-vkeybd
${MAKE} dist-generic || exit 1

cp dist-generic/scummvm/scummvm "${SCRIPT_DIR}/scummvm.elf" || exit 1
cp -r dist-generic/scummvm/data "${SCRIPT_DIR}/data" || exit 1
mkdir -p "${SCRIPT_DIR}/extras" && touch "${SCRIPT_DIR}/extras/.keep" || exit 1
mkdir -p "${SCRIPT_DIR}/icons" && touch "${SCRIPT_DIR}/icons/.keep" || exit 1
mkdir -p "${SCRIPT_DIR}/saves" && touch "${SCRIPT_DIR}/saves/.keep" || exit 1
mkdir -p "${SCRIPT_DIR}/screenshots" && touch "${SCRIPT_DIR}/screenshots/.keep" || exit 1
mkdir -p "${SCRIPT_DIR}/themes" && touch "${SCRIPT_DIR}/themes/.keep" || exit 1
