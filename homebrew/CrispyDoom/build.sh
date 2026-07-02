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

VER=7.1
URL=https://github.com/fabiangreffrath/crispy-doom/archive/refs/tags/crispy-doom-$VER.tar.gz
WAD_DOOM=https://www.doomworld.com/3ddownloads/ports/shareware_doom_iwad.zip
WAD_HERETIC=https://www.doomworld.com/3ddownloads/ports/shareware_heretic_iwad.zip
WAD_HEXEN=https://www.doomworld.com/3ddownloads/ports/shareware_hexen_iwad.zip

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"

if [[ -z "$PS5_PAYLOAD_SDK" ]]; then
    echo "error: PS5_PAYLOAD_SDK is not set"
    exit 1
fi

set -e

source "${PS5_PAYLOAD_SDK}/toolchain/prospero.sh"

TEMPDIR=$(mktemp -d)
trap 'rm -rf -- "$TEMPDIR"' EXIT

# fetch assets
cd $TEMPDIR
curl -o wad_doom.zip "${WAD_DOOM}"
curl -o wad_heretic.zip "${WAD_HERETIC}"
curl -o wad_hexen.zip "${WAD_HEXEN}"
sync
sha256sum wad_doom.zip
sha256sum wad_heretic.zip
sha256sum wad_hexen.zip
unzip wad_doom.zip
unzip wad_heretic.zip
unzip wad_hexen.zip
mv *.WAD "${SCRIPT_DIR}/"

# fetch and build the engine
curl -L -o crispy-doom.tar.gz "${URL}"
tar xf crispy-doom.tar.gz
cd crispy-doom-crispy-doom-$VER

# Forking a ps5 bigapp crashes the kernel
sed -i 's|childpid = fork();|return -1;|g' src/setup/execute.c

./autogen.sh --prefix="${PREFIX}" --host=x86_64-pc-freebsd \
             --disable-doc --disable-zpool --disable-icons \
             --disable-bash-completion
${MAKE}

mv data/doom.png "${SCRIPT_DIR}/"
mv src/crispy-doom "${SCRIPT_DIR}/crispy-doom.elf"
mv src/crispy-doom-setup "${SCRIPT_DIR}/crispy-doom-setup.elf"

mv data/heretic.png "${SCRIPT_DIR}/"
mv src/crispy-heretic "${SCRIPT_DIR}/crispy-heretic.elf"
mv src/crispy-heretic-setup "${SCRIPT_DIR}/crispy-heretic-setup.elf"

mv data/hexen.png "${SCRIPT_DIR}/"
mv src/crispy-hexen "${SCRIPT_DIR}/crispy-hexen.elf"
mv src/crispy-hexen-setup "${SCRIPT_DIR}/crispy-hexen-setup.elf"

mv data/strife.png "${SCRIPT_DIR}/"
mv src/crispy-strife "${SCRIPT_DIR}/crispy-strife.elf"
mv src/crispy-strife-setup "${SCRIPT_DIR}/crispy-strife-setup.elf"
