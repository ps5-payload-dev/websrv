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

#include <dirent.h>
#include <dlfcn.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>

#include <microhttpd.h>

#include "plugin.h"
#include "plugin_api.h"
#include "websrv.h"


typedef const char* (*plugin_register_url_fn)(void);
typedef int (*plugin_handle_request_fn)(plugin_context_t ctx,
					const char* url,
					const char* method,
					const plugin_post_data_t* post,
					plugin_response_fn respond);


static enum MHD_Result
plugin_queue_response(struct MHD_Connection* conn,
		      const plugin_response_data_t* resp) {
  struct MHD_Response* mresp;
  plugin_response_header_t* hdr;
  enum MHD_Result ret = MHD_NO;

  if(!resp || resp->status <= 0) {
    return MHD_NO;
  }

  if(!(mresp=MHD_create_response_from_buffer(resp->body_len, (void*)resp->body,
					     MHD_RESPMEM_MUST_COPY))) {
    return MHD_NO;
  }

  for(hdr=resp->headers; hdr; hdr=hdr->next) {
    if(hdr->key && hdr->val) {
      MHD_add_response_header(mresp, hdr->key, hdr->val);
    }
  }

  ret = websrv_queue_response(conn, (unsigned int)resp->status, mresp);
  MHD_destroy_response(mresp);
  return ret;
}


static int
plugin_respond(plugin_context_t ctx, plugin_response_data_t* resp) {
  struct MHD_Connection* conn = (struct MHD_Connection*)ctx;

  if(!resp) {
    return -1;
  }

  return plugin_queue_response(conn, resp) == MHD_YES ? 0 : -1;
}


typedef struct plugin_entry {
  void* handle;
  char* path;
  char* soname;
  char* url_prefix;
  plugin_handle_request_fn handle_request;
  struct plugin_entry* next;
} plugin_entry_t;


static plugin_entry_t* g_plugins = 0;


static const char* const plugin_scan_roots[] = {
  "plugin",
  "/data/homebrew/websrv/plugin",
  "/mnt/usb0/homebrew/websrv/plugin",
  "/mnt/usb1/homebrew/websrv/plugin",
  "/mnt/usb2/homebrew/websrv/plugin",
  "/mnt/usb3/homebrew/websrv/plugin",
  "/mnt/usb4/homebrew/websrv/plugin",
  "/mnt/usb5/homebrew/websrv/plugin",
  "/mnt/usb6/homebrew/websrv/plugin",
  "/mnt/usb7/homebrew/websrv/plugin",
  "/mnt/ext0/homebrew/websrv/plugin",
  "/mnt/ext1/homebrew/websrv/plugin",
};


static char*
plugin_normalize_url_prefix(const char* url) {
  const char* start = url;
  char* out;
  size_t len;

  while(*start == '/') {
    start++;
  }

  if(!*start) {
    return 0;
  }

  len = strlen(start);
  while(len > 0 && start[len - 1] == '/') {
    len--;
  }

  if(!len) {
    return 0;
  }

  out = malloc(len + 2);
  if(!out) {
    return 0;
  }

  out[0] = '/';
  memcpy(out + 1, start, len);
  out[len + 1] = '\0';
  return out;
}


static int
plugin_is_so_file(const char* name) {
  size_t len = strlen(name);

  if(len < 4) {
    return 0;
  }

  return !strcmp(name + len - 3, ".so");
}


static char*
plugin_soname_from_basename(const char* name) {
  size_t len = strlen(name);
  char* soname;

  if(len < 4 || strcmp(name + len - 3, ".so")) {
    return 0;
  }

  len -= 3;
  soname = malloc(len + 1);
  if(!soname) {
    return 0;
  }

  memcpy(soname, name, len);
  soname[len] = '\0';
  return soname;
}


static int
plugin_url_matches(const char* prefix, const char* url) {
  size_t len = strlen(prefix);

  if(strncmp(url, prefix, len)) {
    return 0;
  }

  return !url[len] || url[len] == '/';
}


static void
plugin_try_load(const char* path, const char* basename) {
  void* handle;
  plugin_register_url_fn register_url;
  plugin_handle_request_fn handle_request;
  const char* raw_url;
  char* url_prefix;
  char* soname;
  char sym_register[256];
  char sym_handle[256];
  plugin_entry_t* entry;

  soname = plugin_soname_from_basename(basename);
  if(!soname) {
    fprintf(stderr, "plugin: %s: invalid plugin filename (expected <soname>.so)\n",
	    path);
    return;
  }

  for(entry=g_plugins; entry; entry=entry->next) {
    if(!strcmp(entry->soname, soname)) {
      fprintf(stderr, "plugin: %s: %s.so already loaded from %s\n",
	      path, soname, entry->path);
      free(soname);
      return;
    }
  }

  snprintf(sym_register, sizeof(sym_register), "%s_plugin_register_url", soname);
  snprintf(sym_handle, sizeof(sym_handle), "%s_plugin_handle_request", soname);

  handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  if(!handle) {
    fprintf(stderr, "plugin: dlopen(%s): %s\n", path, dlerror());
    free(soname);
    return;
  }

  register_url = (plugin_register_url_fn)dlsym(handle, sym_register);
  if(!register_url) {
    fprintf(stderr, "plugin: %s: missing %s: %s\n", path, sym_register, dlerror());
    dlclose(handle);
    free(soname);
    return;
  }

  handle_request = (plugin_handle_request_fn)dlsym(handle, sym_handle);
  if(!handle_request) {
    fprintf(stderr, "plugin: %s: missing %s: %s\n", path, sym_handle, dlerror());
    dlclose(handle);
    free(soname);
    return;
  }

  raw_url = register_url();
  if(!raw_url || !*raw_url) {
    fprintf(stderr, "plugin: %s: %s returned empty string\n", path, sym_register);
    dlclose(handle);
    free(soname);
    return;
  }

  url_prefix = plugin_normalize_url_prefix(raw_url);
  if(!url_prefix) {
    fprintf(stderr, "plugin: %s: invalid URL from %s: %s\n",
	    path, sym_register, raw_url);
    dlclose(handle);
    free(soname);
    return;
  }

  for(entry=g_plugins; entry; entry=entry->next) {
    if(!strcmp(entry->url_prefix, url_prefix)) {
      fprintf(stderr, "plugin: %s: URL %s already registered by %s\n",
	      path, url_prefix, entry->path);
      free(url_prefix);
      free(soname);
      dlclose(handle);
      return;
    }
  }

  entry = calloc(1, sizeof(plugin_entry_t));
  if(!entry) {
    free(url_prefix);
    free(soname);
    dlclose(handle);
    return;
  }

  entry->handle = handle;
  entry->path = strdup(path);
  entry->soname = soname;
  entry->url_prefix = url_prefix;
  entry->handle_request = handle_request;
  entry->next = g_plugins;
  g_plugins = entry;

  printf("plugin: loaded %s at %s\n", path, url_prefix);
}


static void
plugin_scan_dir(const char* dir) {
  DIR* dp;
  struct dirent* ent;
  char path[PATH_MAX];
  struct stat st;

  dp = opendir(dir);
  if(!dp) {
    return;
  }

  while((ent=readdir(dp))) {
    if(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
      continue;
    }

    snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);

    if(stat(path, &st) < 0) {
      continue;
    }

    if(S_ISDIR(st.st_mode)) {
      plugin_scan_dir(path);
      continue;
    }

    if(S_ISREG(st.st_mode) && plugin_is_so_file(ent->d_name)) {
      plugin_try_load(path, ent->d_name);
    }
  }

  closedir(dp);
}


void
plugin_unload_all(void) {
  plugin_entry_t* entry;
  plugin_entry_t* next;

  for(entry=g_plugins; entry; entry=next) {
    next = entry->next;
    dlclose(entry->handle);
    free(entry->path);
    free(entry->soname);
    free(entry->url_prefix);
    free(entry);
  }

  g_plugins = 0;
}


void
plugin_load_all(void) {
  size_t i;

  plugin_unload_all();

  for(i=0; i<sizeof(plugin_scan_roots)/sizeof(plugin_scan_roots[0]); i++) {
    plugin_scan_dir(plugin_scan_roots[i]);
  }
}


enum MHD_Result
plugin_request(struct MHD_Connection* conn, const char* url,
	       const char* method, const plugin_post_data_t* post) {
  plugin_entry_t* entry;
  plugin_entry_t* match = 0;
  size_t best = 0;
  size_t len;

  for(entry=g_plugins; entry; entry=entry->next) {
    len = strlen(entry->url_prefix);
    if(plugin_url_matches(entry->url_prefix, url) && len >= best) {
      match = entry;
      best = len;
    }
  }

  if(!match) {
    return MHD_NO;
  }

  return match->handle_request((plugin_context_t)conn, url, method, post,
			       plugin_respond) == 0 ? MHD_YES : MHD_NO;
}


enum MHD_Result
plugin_list_request(struct MHD_Connection* conn) {
  plugin_entry_t* entry;
  char* body;
  size_t cap = 4096;
  size_t len = 0;
  enum MHD_Result ret = MHD_NO;
  struct MHD_Response* resp;

  body = malloc(cap);
  if(!body) {
    return MHD_NO;
  }

  len += snprintf(body + len, cap - len,
		  "<!DOCTYPE html>"
		  "<html><head><title>Plugins</title></head>"
		  "<body><h1>Plugins</h1><ul>");

  for(entry=g_plugins; entry; entry=entry->next) {
    const char* label = entry->url_prefix + 1;
    int n = snprintf(body + len, cap - len,
		     "<li><a href=\"%s\">%s</a></li>",
		     entry->url_prefix, label);

    if(n < 0 || (size_t)n >= cap - len) {
      free(body);
      return MHD_NO;
    }
    len += (size_t)n;
  }

  len += snprintf(body + len, cap - len, "</ul></body></html>");

  if((resp=MHD_create_response_from_buffer(len, body, MHD_RESPMEM_MUST_FREE))) {
    MHD_add_response_header(resp, MHD_HTTP_HEADER_CONTENT_TYPE, "text/html");
    ret = websrv_queue_response(conn, MHD_HTTP_OK, resp);
    MHD_destroy_response(resp);
  } else {
    free(body);
  }

  return ret;
}
