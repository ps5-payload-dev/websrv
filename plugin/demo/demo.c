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

#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "plugin_api.h"


#define DEMO_CTX_SLOTS 64


static const char PAGE[] =
  "<html><head><title>websrv plugin demo</title></head>"
  "<body>"
  "<h1>plugin/demo</h1>"
  "<form method=\"POST\">"
  "<label>msg <input name=\"msg\" value=\"hello\"></label>"
  "<button type=\"submit\">POST</button>"
  "</form>"
  "</body></html>";

static const char BODY_429[] = "Too Many Requests";


typedef struct demo_request_state {
  plugin_context_t ctx;
  plugin_response_header_t hdr;
  plugin_response_data_t resp;
  char json_body[4096];
} demo_request_state_t;


static demo_request_state_t g_demo_state[DEMO_CTX_SLOTS];


static demo_request_state_t*
demo_state_for(plugin_context_t ctx) {
  size_t i;
  demo_request_state_t* free_slot = 0;

  for(i=0; i<DEMO_CTX_SLOTS; i++) {
    if(g_demo_state[i].ctx == ctx) {
      return &g_demo_state[i];
    }
    if(!free_slot && !g_demo_state[i].ctx) {
      free_slot = &g_demo_state[i];
    }
  }

  if(!free_slot) {
    return 0;
  }

  free_slot->ctx = ctx;
  return free_slot;
}


static void
demo_state_release(plugin_context_t ctx) {
  size_t i;

  for(i=0; i<DEMO_CTX_SLOTS; i++) {
    if(g_demo_state[i].ctx == ctx) {
      memset(&g_demo_state[i], 0, sizeof(g_demo_state[i]));
      return;
    }
  }
}


static plugin_response_data_t*
demo_build_response(demo_request_state_t* state, int status,
                    const char* mime, const uint8_t* body, size_t body_len) {
  state->hdr.key = "Content-Type";
  state->hdr.val = mime;
  state->hdr.next = 0;

  state->resp.status = status;
  state->resp.headers = &state->hdr;
  state->resp.body = body;
  state->resp.body_len = body_len;
  return &state->resp;
}


static plugin_response_data_t*
demo_too_many_requests(void) {
  static plugin_response_header_t hdr;
  static plugin_response_data_t resp;

  hdr.key = "Content-Type";
  hdr.val = "text/plain";
  hdr.next = 0;
  resp.status = PLUGIN_HTTP_TOO_MANY_REQUESTS;
  resp.headers = &hdr;
  resp.body = (const uint8_t*)BODY_429;
  resp.body_len = sizeof(BODY_429) - 1;
  return &resp;
}


const char*
demo_plugin_register_url(void) {
  return "plugin/demo";
}


int
demo_plugin_handle_request(plugin_context_t ctx, const char* url,
                           const char* method, const plugin_post_data_t* post,
                           plugin_response_fn respond) {
  demo_request_state_t* state;
  size_t len;
  int first;
  int ret;

  (void)url;

  state = demo_state_for(ctx);
  if(!state) {
    return respond(ctx, demo_too_many_requests());
  }

  if(!strcmp(method, PLUGIN_METHOD_GET) ||
     !strcmp(method, PLUGIN_METHOD_HEAD)) {
    ret = respond(ctx, demo_build_response(state, PLUGIN_HTTP_OK, "text/html",
                                           (const uint8_t*)PAGE,
                                           sizeof(PAGE) - 1));
    demo_state_release(ctx);
    return ret;
  }

  if(!strcmp(method, PLUGIN_METHOD_POST)) {
    first = 1;
    len = snprintf(state->json_body, sizeof(state->json_body),
                   "{\"ctx\":%" PRId64 ",\"fields\":{",
                   (int64_t)(intptr_t)ctx);
    for(; post && len < sizeof(state->json_body) - 64; post=post->next) {
      if(!first) {
        len += snprintf(state->json_body + len,
                        sizeof(state->json_body) - len, ",");
      }
      first = 0;
      len += snprintf(state->json_body + len,
                      sizeof(state->json_body) - len,
                      "\"%s\":\"", post->key);
      if(post->val && post->len) {
        len += snprintf(state->json_body + len,
                        sizeof(state->json_body) - len, "%.*s",
                        (int)post->len, (const char*)post->val);
      }
      len += snprintf(state->json_body + len,
                      sizeof(state->json_body) - len, "\"");
    }
    len += snprintf(state->json_body + len,
                    sizeof(state->json_body) - len, "}}");
    ret = respond(ctx, demo_build_response(state, PLUGIN_HTTP_OK,
                                           "application/json",
                                           (const uint8_t*)state->json_body,
                                           len));
    demo_state_release(ctx);
    return ret;
  }

  demo_state_release(ctx);
  return 1;
}
