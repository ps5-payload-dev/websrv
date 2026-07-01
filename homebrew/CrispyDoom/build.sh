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

# Build script by Alex Free, based on template of other projects.
SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"

if [[ -z "$PS5_PAYLOAD_SDK" ]]; then
    if [ -z "${PS5_PAYLOAD_SDK}" ]; then
	export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk
    fi
fi

source "${PS5_PAYLOAD_SDK}/toolchain/prospero.sh" || exit 1

cd "${SCRIPT_DIR}"
# Executables are included with sdk, just copy it.
# Each engine uses it's own config files, and by putting them in their own folder it makes it nicer to manage this by the UI.
# Download sharewares when available with wget, that is already installed as a neccesary dep in the toolchain building process.
rm -rf doom heretic hexen strife payloads

if [[ "$1" == "clean" ]]; then
     exit 0
fi

set -e

# We put all payloads in one dir to cleanup UX.
mkdir payloads

# All these tmp dirs are because we need somewhere to extract/play MIDI tracks found in WADs, and we don't have a /tmp on PS5. This is setup by envars in the javascript.
mkdir -p doom/tmp
mkdir -p heretic/tmp
mkdir -p hexen/tmp
# No shareware of strife this can run, the demo is very different from retail and not compatible: https://github.com/fabiangreffrath/crispy-doom/blob/master/README.Strife.md#what-are-some-known-problems.
mkdir -p strife/tmp

# Make music-packs.
mkdir doom/music-packs
mkdir heretic/music-packs
mkdir hexen/music-packs

# Make autoload
mkdir doom/autoload
mkdir heretic/autoload
mkdir hexen/autoload
mkdir strife/autoload

# Shareware site: https://www.doomworld.com/classicdoom/info/shareware.php is cool, but need to spoof a valid UA or download gets denied.

# DOOM.
cp -v "${PS5_SYSROOT}/${PS5_HBROOT}/bin/crispy-doom" payloads/crispy-doom.elf
cp -v "${PS5_SYSROOT}/${PS5_HBROOT}/bin/crispy-doom-setup" payloads/crispy-doom-setup.elf
# Doom v1.9 shareware. No Doom 2 shareware was ever made btw.
wget --user-agent="Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36" \
     https://www.doomworld.com/3ddownloads/ports/shareware_doom_iwad.zip \
     -O doom/shareware_doom_iwad.zip

unzip doom/shareware_doom_iwad.zip -d doom
rm doom/shareware_doom_iwad.zip
# Crispy.
cp -v default-configs/crispy-doom.cfg doom
# Vanilla.
cp -v default-configs/doom.cfg doom

# Heretic.
cp -v "${PS5_SYSROOT}/${PS5_HBROOT}/bin/crispy-heretic" payloads/crispy-heretic.elf
cp -v "${PS5_SYSROOT}/${PS5_HBROOT}/bin/crispy-heretic-setup" payloads/crispy-heretic-setup.elf
# Heretic v1.2 shareware.
wget --user-agent="Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36" \
     https://www.doomworld.com/3ddownloads/ports/shareware_heretic_iwad.zip \
     -O heretic/shareware_heretic_iwad.zip

unzip heretic/shareware_heretic_iwad.zip -d heretic
rm heretic/shareware_heretic_iwad.zip
# Crispy.
cp -v default-configs/crispy-heretic.cfg heretic
# Vanilla.
cp -v default-configs/heretic.cfg heretic

# Hexen.
cp -v "${PS5_SYSROOT}/${PS5_HBROOT}/bin/crispy-hexen" payloads/crispy-hexen.elf
cp -v "${PS5_SYSROOT}/${PS5_HBROOT}/bin/crispy-hexen-setup" payloads/crispy-hexen-setup.elf
# Hexen shareware v1.1.
wget --user-agent="Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36" \
     https://www.doomworld.com/3ddownloads/ports/shareware_hexen_iwad.zip \
     -O hexen/shareware_hexen_iwad.zip

unzip hexen/shareware_hexen_iwad.zip -d hexen
rm hexen/shareware_hexen_iwad.zip
# Crispy.
cp -v default-configs/crispy-hexen.cfg hexen
# Vanilla.
cp -v default-configs/hexen.cfg hexen

# Strife.
cp -v "${PS5_SYSROOT}/${PS5_HBROOT}/bin/crispy-strife" payloads/crispy-strife.elf
cp -v "${PS5_SYSROOT}/${PS5_HBROOT}/bin/crispy-strife-setup" payloads/crispy-strife-setup.elf
# AGAIN, no demo/shareware usable since this is a RE of lost source code never released, for the retail DOS version.
# Crispy.
cp -v default-configs/crispy-strife.cfg strife
# Vanilla.
cp -v default-configs/strife.cfg strife
