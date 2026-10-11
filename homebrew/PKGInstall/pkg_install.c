/* Copyright (C) 2025 John Törnblom

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING. If not, see
<http://www.gnu.org/licenses/>.  */

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct pkg_metadata {
  const char* uri;
  const char* ex_uri;
  const char* playgo_scenario_id;
  const char* content_id;
  const char* content_name;
  const char* icon_url;
  uint32_t slot;
  uint32_t is_playgo_enabled;
} pkg_metadata_t;

_Static_assert(sizeof(pkg_metadata_t) == 0x38,
               "sceAppInstUtil metadata ABI mismatch");


typedef struct pkg_info {
  char content_id[48];
  int type;
  int platform;
} pkg_info_t;


typedef struct playgo_info {
  char languages[30][8];
  char scenario_ids[64][3];
  char content_ids[64][48];
  long unknown[810];
} playgo_info_t;

_Static_assert(sizeof(playgo_info_t) == 0x2700,
               "sceAppInstUtil PlayGoInfo ABI mismatch");


int sceAppInstUtilInitialize(void);
int sceAppInstUtilInstallByPackage(const pkg_metadata_t*, pkg_info_t*, playgo_info_t*);


int
main(int argc, char** argv) {
  char install_path[PATH_MAX + sizeof("/user")];
  const char* uri;
  playgo_info_t playgoinfo = {0};
  pkg_info_t pkginfo = {0};
  pkg_metadata_t metainfo = {
    .playgo_scenario_id = "",
    .content_name = "",
    .content_id = "",
    .icon_url = "",
    .ex_uri = "",
    .uri = "",
    .slot = 0,
    .is_playgo_enabled = 0
  };

  if(argc < 2) {
    printf("Usage: %s URL\n", argv[0]);
    return -1;
  }

  if(sceAppInstUtilInitialize()) {
    return -1;
  }

  uri = argv[1];
  if(!strncmp(uri, "/data/", 6)) {
    snprintf(install_path, sizeof(install_path), "/user%s", uri);
    uri = install_path;
  }
  metainfo.uri = uri;

  return sceAppInstUtilInstallByPackage(&metainfo, &pkginfo, &playgoinfo);
}

