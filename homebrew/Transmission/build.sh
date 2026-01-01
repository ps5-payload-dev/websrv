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

VER="4.0.6"
URL="https://github.com/transmission/transmission/releases/download/$VER/transmission-$VER.tar.xz"
if [[ -z "$PS5_PAYLOAD_SDK" ]]; then
    echo "error: PS5_PAYLOAD_SDK is not set"
    exit 1
fi

source "${PS5_PAYLOAD_SDK}/toolchain/prospero.sh" || exit 1

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"

TEMPDIR=$(mktemp -d)
trap 'rm -rf -- "$TEMPDIR"' EXIT

wget -O $TEMPDIR/transmission.tar.xz "${URL}" || exit 1
tar xf $TEMPDIR/transmission.tar.xz -C $TEMPDIR || exit 1

cd $TEMPDIR/transmission-$VER || exit 1

${CMAKE} -DCMAKE_BUILD_TYPE=Release \
	 -DCMAKE_EXE_LINKER_FLAGS="-ldht -lzstd -lz" \
	 -DENABLE_CLI=NO \
	 -DENABLE_DAEMON=YES \
	 -DENABLE_GTK=NO \
	 -DENABLE_MAC=NO \
	 -DENABLE_QT=NO \
	 -DENABLE_NLS=NO \
	 -DINSTALL_DOC=NO \
	 -DREBUILD_WEB=NO \
	 -DENABLE_TESTS=NO \
	 -DENABLE_UTILS=NO \
	 -DINSTALL_LIB=YES \
	 -DUSE_SYSTEM_B64=YES \
	 -DUSE_SYSTEM_DEFLATE=YES \
	 -DUSE_SYSTEM_DHT=YES \
	 -DUSE_SYSTEM_EVENT2=YES \
	 -DUSE_SYSTEM_MINIUPNPC=YES \
	 -DUSE_SYSTEM_NATPMP=YES \
	 -DUSE_SYSTEM_PSL=YES \
	 -DUSE_SYSTEM_UTP=YES \
	 -DWITH_CRYPTO=openssl \
	 -B build \
         -S .
${MAKE} -C build

rm -rf "${SCRIPT_DIR}/public_html"
mv $TEMPDIR/transmission-$VER/build/daemon/transmission-daemon "${SCRIPT_DIR}/transmission-daemon.elf" || exit 1
mv $TEMPDIR/transmission-$VER/web/public_html "${SCRIPT_DIR}/" || exit 1
