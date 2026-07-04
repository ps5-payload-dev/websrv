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

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"

if [[ -z "$PS5_PAYLOAD_SDK" ]]; then
    if [ -z "${PS5_PAYLOAD_SDK}" ]; then
	export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk
    fi
fi

source "${PS5_PAYLOAD_SDK}/toolchain/prospero.sh" || exit 1

cd "${SCRIPT_DIR}"
rm -rf shareware registered payloads

if [[ "$1" == "clean" ]]; then
     exit 0
fi

set -e

mkdir shareware
mkdir registered
mkdir payloads

# Shareware site: https://beta.wolf3d.net/games/29/downloads/4477.

cp -v ${PS5_SYSROOT}/${PS5_HBROOT}/bin/wolf3d-registered payloads/wolf3d-registered.elf
cp -v ${PS5_SYSROOT}/${PS5_HBROOT}/bin/wolf3d-shareware payloads/wolf3d-shareware.elf

# Wolfenstien 3D v1.4 shareware.
mkdir tmp
wget -O tmp/shareware.zip "https://beta.wolf3d.net/file-download/download/private/4457"
unzip tmp/shareware.zip -d tmp
mv tmp/*.WL1 shareware
rm -rf tmp