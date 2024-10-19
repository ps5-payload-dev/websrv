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
URL="https://github.com/ps5-payload-dev/FBNeo/archive/refs/heads/$VER.tar.gz"

SCRIPT_PATH="${BASH_SOURCE[0]}"
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"

if [[ -z "$PS5_PAYLOAD_SDK" ]]; then
    echo "error: PS5_PAYLOAD_SDK is not set"
    exit 1
fi

source "${PS5_PAYLOAD_SDK}/toolchain/prospero.sh" || exit 1

TEMPDIR=$(mktemp -d)
trap 'rm -rf -- "$TEMPDIR"' EXIT
wget -O $TEMPDIR/fbneo.tar.gz "${URL}" || exit 1
tar xf $TEMPDIR/fbneo.tar.gz -C $TEMPDIR || exit 1

cd $TEMPDIR/FBNeo-$VER || exit 1
${MAKE} sdl2 || exit 1

cp fbneo "${SCRIPT_DIR}/fbneo.elf"
