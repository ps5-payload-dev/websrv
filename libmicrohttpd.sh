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

LIB_URL="https://ftp.gnu.org/gnu/libmicrohttpd/"
LIB_VER="1.0.1"

if [[ -z "$PS5_PAYLOAD_SDK" ]]; then
    echo "error: PS5_PAYLOAD_SDK is not set"
    exit 1
fi

source ${PS5_PAYLOAD_SDK}/toolchain/prospero.sh || exit 1

TEMPDIR=$(mktemp -d)
trap 'rm -rf -- "$TEMPDIR"' EXIT

wget -O $TEMPDIR/lib.tar.gz $LIB_URL/libmicrohttpd-$LIB_VER.tar.gz || exit 1
tar xf $TEMPDIR/lib.tar.gz -C $TEMPDIR || exit 1

# Optimizing with default -O2 causes issues
export CFLAGS="-O1"

cd $TEMPDIR/libmicrohttpd-$LIB_VER
./configure --prefix="${PS5_HBROOT}" --host=x86_64 \
	    --disable-shared --enable-static \
	    --disable-curl --disable-examples 
${MAKE} install DESTDIR="${PS5_PAYLOAD_SDK}/target"
