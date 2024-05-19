#!/usr/bin/env python3
# encoding: utf-8
# Copyright (C) 2024 John Törnblom
#
# This program is free software; you can redistribute it and/or modify it
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

import argparse
import string


tmpl = string.Template('''
void asset_register(const char* path, void* data, unsigned long size);

static unsigned char data[] = $data;

__attribute__((constructor)) static void
constructor(void) {
  asset_register("/$path", data, sizeof(data));
}
''')


def gen_data(filename):
    yield '{\n  '

    with open(filename, mode='rb') as f:
        for n, b in enumerate(f.read(), 1):
            yield hex(b)
            yield ', '

            if n % 16 == 0:
                yield '\n  '

    yield '\n}'

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--path', default=None)
    parser.add_argument('FILE')
    args = parser.parse_args()

    if args.path is None:
        args.path = args.FILE

    data = ''.join(gen_data(args.FILE))
    print(tmpl.substitute(data=data, path=args.path))

