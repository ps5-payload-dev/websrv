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

VIDEO_URL=http://www.duke4.org/files/nightfright/hrp/duke3d_hrp.zip
AUDIO_URL=http://www.duke4.org/files/nightfright/music/duke3d_music-sc55.zip

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"

wget -qO- $VIDEO_URL | bsdtar -xvf- -C $SCRIPT_DIR
wget -qO- $AUDIO_URL | bsdtar -xvf- -C $SCRIPT_DIR
