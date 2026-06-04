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

#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * @file plugin_api.h
 *
 * Native plugin API for websrv. Plugins are shared objects named `<soname>.so`.
 * The loader resolves these exported symbols (example for `demo.so`):
 *
 *   demo_plugin_register_url
 *   demo_plugin_handle_request
 *
 * Plugins need only this header; libmicrohttpd is linked in the host.
 *
 * Return convention: **0 = OK**, non-zero = error or not handled.
 */

/**
 * Per-request context. In the host this is `struct MHD_Connection *`, cast to
 * `void *`. Each concurrent request has its own value; use it to key per-request
 * state. Valid for the duration of `<soname>_plugin_handle_request`.
 */
typedef void* plugin_context_t;

/* 2xx success */
#define PLUGIN_HTTP_OK                        200
#define PLUGIN_HTTP_CREATED                   201
#define PLUGIN_HTTP_ACCEPTED                  202
#define PLUGIN_HTTP_NO_CONTENT                204

/* 3xx redirection */
#define PLUGIN_HTTP_MOVED_PERMANENTLY         301
#define PLUGIN_HTTP_FOUND                     302
#define PLUGIN_HTTP_SEE_OTHER                 303
#define PLUGIN_HTTP_NOT_MODIFIED              304

/* 4xx client error */
#define PLUGIN_HTTP_BAD_REQUEST               400
#define PLUGIN_HTTP_UNAUTHORIZED              401
#define PLUGIN_HTTP_FORBIDDEN                 403
#define PLUGIN_HTTP_NOT_FOUND                 404
#define PLUGIN_HTTP_METHOD_NOT_ALLOWED        405
#define PLUGIN_HTTP_NOT_ACCEPTABLE            406
#define PLUGIN_HTTP_CONFLICT                  409
#define PLUGIN_HTTP_GONE                      410
#define PLUGIN_HTTP_PAYLOAD_TOO_LARGE         413
#define PLUGIN_HTTP_UNSUPPORTED_MEDIA_TYPE    415
#define PLUGIN_HTTP_TOO_MANY_REQUESTS         429

/* 5xx server error */
#define PLUGIN_HTTP_INTERNAL_SERVER_ERROR     500
#define PLUGIN_HTTP_NOT_IMPLEMENTED           501
#define PLUGIN_HTTP_BAD_GATEWAY               502
#define PLUGIN_HTTP_SERVICE_UNAVAILABLE       503
#define PLUGIN_HTTP_GATEWAY_TIMEOUT           504

/** HTTP method strings passed to `<soname>_plugin_handle_request`. */
#define PLUGIN_METHOD_GET                     "GET"
#define PLUGIN_METHOD_POST                    "POST"
#define PLUGIN_METHOD_HEAD                    "HEAD"

/**
 * One field from a parsed POST body (`application/x-www-form-urlencoded` or
 * `multipart/form-data`). Fields are a singly linked list via @p next.
 */
typedef struct plugin_post_data {
  const char* key;                   /**< field name */
  const uint8_t* val;                /**< field value (not necessarily NUL-terminated) */
  size_t len;                        /**< length of @p val in bytes */
  struct plugin_post_data* next;     /**< next field, or NULL */
} plugin_post_data_t;

/**
 * One HTTP response header. Headers are a singly linked list via @p next.
 */
typedef struct plugin_response_header {
  const char* key;                       /**< header name, e.g. `"Content-Type"` */
  const char* val;                       /**< header value, e.g. `"text/html"` */
  struct plugin_response_header* next;   /**< next header, or NULL */
} plugin_response_header_t;

/**
 * HTTP response descriptor passed to @p respond. Use PLUGIN_HTTP_* for @p status.
 * @p body and header strings must remain valid until @p respond returns (the host
 * copies the body before the callback returns).
 */
typedef struct plugin_response_data {
  int status;                          /**< HTTP status code (e.g. PLUGIN_HTTP_OK) */
  plugin_response_header_t* headers;   /**< response headers, or NULL */
  const uint8_t* body;                 /**< response body, or NULL for empty body */
  size_t body_len;                     /**< length of @p body in bytes */
} plugin_response_data_t;

/**
 * Host-provided callback to queue an HTTP response.
 *
 * @param ctx  Request context (`plugin_context_t` for this connection).
 * @param resp Response to send; may be stack- or slot-allocated in the plugin.
 * @return 0 on success, non-zero on failure.
 */
typedef int (*plugin_response_fn)(plugin_context_t ctx,
				    plugin_response_data_t* resp);

/**
 * `<soname>_plugin_register_url` — return the URL prefix owned by this plugin.
 *
 * @return Prefix without a leading slash, e.g. `"plugin/demo"` serves
 *         `/plugin/demo` and sub-paths. Must remain valid for the lifetime of
 *         the loaded plugin (typically a string literal).
 *
 * Example:
 * @code
 * const char *demo_plugin_register_url(void) {
 *   return "plugin/demo";
 * }
 * @endcode
 */

/**
 * `<soname>_plugin_handle_request` — handle a request routed to this plugin.
 *
 * @param ctx     Per-request context; use to key temporary state.
 * @param url     Request path with leading slash, e.g. `"/plugin/demo/page"`.
 * @param method  HTTP method (`PLUGIN_METHOD_GET`, `PLUGIN_METHOD_POST`, …).
 * @param post    Parsed POST form fields, or NULL for GET/HEAD and POST with
 *                no body fields.
 * @param respond Host callback; call with the response when handled.
 * @return 0 if the request was handled and @p respond succeeded; non-zero if
 *         the request is not handled or @p respond failed. Release per-request
 *         buffers after @p respond returns.
 *
 * Example:
 * @code
 * int demo_plugin_handle_request(plugin_context_t ctx, const char *url,
 *                                const char *method,
 *                                const plugin_post_data_t *post,
 *                                plugin_response_fn respond) {
 *   plugin_response_data_t resp = { .status = PLUGIN_HTTP_OK, ... };
 *   return respond(ctx, &resp);
 * }
 * @endcode
 */
