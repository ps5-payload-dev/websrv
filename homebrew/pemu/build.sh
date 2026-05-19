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

URL="https://github.com/Cpasjuste/pemu.git"

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"

if [[ -z "$PS5_PAYLOAD_SDK" ]]; then
    echo "error: PS5_PAYLOAD_SDK is not set"
    exit 1
fi

source "${PS5_PAYLOAD_SDK}/toolchain/prospero.sh" || exit 1

TEMPDIR=$(mktemp -d)
trap 'rm -rf -- "$TEMPDIR"' EXIT

set -e

cd "${TEMPDIR}"
git clone --recurse-submodules $URL
cd pemu

mkdir cmake-build && cd cmake-build
${CMAKE} -DPLATFORM_PS5=ON -DOPTION_MPV_PLAYER=OFF -DCMAKE_BUILD_TYPE=Release ..

${MAKE} pfbneo.deps
${MAKE} pfbneo_ps5_release
${MAKE} pgen_ps5_release
${MAKE} pnes_ps5_release
${MAKE} psnes_ps5_release
${MAKE} pgba_ps5_release

mv *.zip "${SCRIPT_DIR}/"
