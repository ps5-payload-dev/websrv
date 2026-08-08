/* Copyright (C) 2026 John Törnblom

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
#include <sys/types.h>


/**
 * Opaque type representing the response to a remote HTTP request.
 **/
typedef struct http_response http_response_t;


/**
 * Callback invoked once per header by http_response_foreach_header().
 **/
typedef void (*http_header_cb_t)(const char* name, const char* value,
				 void* ctx);


/**
 * Fetch the contents of a URL into memory.
 *
 * Returns a NUL-terminated buffer owned by the caller, which must be released
 * with free(), or NULL if the request failed. If size is non-NULL, the number
 * of bytes fetched (excluding the terminating NUL) is stored there.
 **/
uint8_t* http_get(const char* url, size_t* size);


/**
 * Issue a request to a remote server.
 *
 * Blocks until the response headers have been received, after which the body
 * is transferred as it is pulled out with http_response_read(). Returns NULL
 * if no response was received.
 *
 * A response is not thread safe, and both http_request() and
 * http_response_read() block, so a response should be used by one thread at a
 * time, and by a thread that can afford to wait for the remote server.
 *
 * The method defaults to GET when NULL. The headers argument is an optional
 * NULL-terminated array of "Name: value" strings sent with the request.
 *
 * The returned response must be released with http_response_free().
 **/
http_response_t* http_request(const char* method, const char* url,
			      const char* const* headers);


/**
 * Get the status code sent by the remote server.
 **/
long http_response_status(const http_response_t* resp);


/**
 * Get the value of a response header, or NULL if the remote server did not
 * send it. Header names are matched case-insensitively.
 **/
const char* http_response_header(const http_response_t* resp,
				 const char* name);


/**
 * Invoke a callback once for each header sent by the remote server, in the
 * order they were received.
 **/
void http_response_foreach_header(const http_response_t* resp,
				  http_header_cb_t cb, void* ctx);


/**
 * Read up to size bytes of the response body.
 *
 * Blocks until at least one byte is available. Returns the number of bytes
 * read, 0 once the whole body has been read, or -1 if the transfer ended
 * prematurely.
 **/
ssize_t http_response_read(http_response_t* resp, void* buf, size_t size);


/**
 * Release the resources used by a response.
 *
 * Can be called before the whole body has been read, in which case the
 * remaining part of the transfer is aborted.
 **/
void http_response_free(http_response_t* resp);
