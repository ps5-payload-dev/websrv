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

VER="master"
URL="https://github.com/ps5-payload-dev/linkdev/archive/refs/heads/$VER.tar.gz"

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"

if [[ -z "$PS5_PAYLOAD_SDK" ]]; then
    echo "error: PS5_PAYLOAD_SDK is not set"
    exit 1
fi

source "${PS5_PAYLOAD_SDK}/toolchain/prospero.sh" || exit 1

TEMPDIR=$(mktemp -d)
trap 'rm -rf -- "$TEMPDIR"' EXIT

wget -O $TEMPDIR/linkdev.tar.gz "${URL}" || exit 1
tar xf $TEMPDIR/linkdev.tar.gz -C $TEMPDIR || exit 1

cd $TEMPDIR/linkdev-$VER || exit 1
${MAKE} || exit 1

mv $TEMPDIR/linkdev-$VER/LinkDev.elf "${SCRIPT_DIR}/LinkDev.elf" || exit 1
mv $TEMPDIR/linkdev-$VER/homebrew.js "${SCRIPT_DIR}/homebrew.js" || exit 1
mv $TEMPDIR/linkdev-$VER/sce_sys "${SCRIPT_DIR}/sce_sys" || exit 1
