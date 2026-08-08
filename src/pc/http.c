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

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <curl/curl.h>

#include "http.h"


/**
 * Give up if the remote server does not accept the connection in time.
 **/
#define HTTP_CONNECT_TIMEOUT 15


/**
 * Give up if the remote server transfers less than HTTP_STALL_LIMIT bytes
 * per second during HTTP_STALL_TIMEOUT seconds.
 **/
#define HTTP_STALL_LIMIT      1
#define HTTP_STALL_TIMEOUT   60


/**
 * Maximum number of redirects to follow before giving up.
 **/
#define HTTP_MAX_REDIRECTS    8


/**
 * How long to wait for socket activity before checking on libcurl again.
 **/
#define HTTP_POLL_TIMEOUT  1000


/**
 * Amount of body data buffered between two reads. Must be larger than
 * HTTP_CURL_BUFFER_SIZE, so a chunk handed over by libcurl always fits.
 **/
#define HTTP_BUFFER_SIZE      0x10000
#define HTTP_CURL_BUFFER_SIZE 0x4000


/**
 * A header sent by a remote server.
 **/
typedef struct http_header {
  char               *name;
  char               *value;
  struct http_header *next;
} http_header_t;


/**
 * State of an ongoing request.
 *
 * The transfer is driven by the thread reading the response, one chunk at a
 * time, so no data is fetched until somebody asks for it, and no more than
 * HTTP_BUFFER_SIZE bytes are ever held in memory. libcurl is told to pause
 * whenever it produces data faster than it is read.
 **/
struct http_response {
  CURLM             *multi;
  CURL              *curl;
  struct curl_slist *reqhdrs;

  int                ready;  // response headers have been received
  int                done;   // transfer has run to completion
  int                failed; // transfer did not deliver a complete response
  int                skip;   // discard body, another response follows
  int                paused; // libcurl is waiting for the buffer to drain

  long               status;
  http_header_t     *headers;
  http_header_t     *tail;

  char               buf[HTTP_BUFFER_SIZE];
  size_t             len;
  size_t             off;
};


/**
 * Buffer used by http_get().
 **/
typedef struct http_buffer {
  uint8_t *data;
  size_t   size;
} http_buffer_t;


static pthread_once_t g_curl_once = PTHREAD_ONCE_INIT;


static void
http_curl_init(void) {
  curl_global_init(CURL_GLOBAL_DEFAULT);
}


/**
 * Only let libcurl speak http, both for the request itself and for any
 * redirect it follows. Without this, a URL like file:///etc/passwd would be
 * served by the proxy.
 **/
static void
http_curl_restrict(CURL *curl) {
#if LIBCURL_VERSION_NUM >= 0x075500 // 7.85.0
  curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http,https");
  curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "http,https");
#else
  curl_easy_setopt(curl, CURLOPT_PROTOCOLS,
		   (long)(CURLPROTO_HTTP | CURLPROTO_HTTPS));
  curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS,
		   (long)(CURLPROTO_HTTP | CURLPROTO_HTTPS));
#endif
}


static void
http_headers_free(http_header_t *hdr) {
  http_header_t *next;

  while(hdr) {
    next = hdr->next;
    free(hdr->name);
    free(hdr->value);
    free(hdr);
    hdr = next;
  }
}


/**
 * Append a "Name: value" line received from a remote server.
 **/
static void
http_headers_append(http_response_t *resp, const char *line, size_t len) {
  http_header_t *hdr;
  const char *val;
  size_t nlen;
  size_t vlen;

  if(!(val=memchr(line, ':', len))) {
    return;
  }

  nlen = val - line;
  val++;
  vlen = len - nlen - 1;

  while(vlen && (*val == ' ' || *val == '\t')) {
    val++;
    vlen--;
  }
  while(vlen && (val[vlen-1] == '\r' || val[vlen-1] == '\n' ||
		 val[vlen-1] == ' ' || val[vlen-1] == '\t')) {
    vlen--;
  }
  while(nlen && (line[nlen-1] == ' ' || line[nlen-1] == '\t')) {
    nlen--;
  }

  if(!nlen || !(hdr=calloc(1, sizeof(http_header_t)))) {
    return;
  }

  if(!(hdr->name=strndup(line, nlen)) || !(hdr->value=strndup(val, vlen))) {
    free(hdr->name);
    free(hdr->value);
    free(hdr);
    return;
  }

  if(resp->tail) {
    resp->tail->next = hdr;
  } else {
    resp->headers = hdr;
  }
  resp->tail = hdr;
}


/**
 * Collect the headers sent by a remote server.
 *
 * libcurl invokes this for informational responses and for each redirect it
 * follows, so headers are reset on every status line, and the response is
 * only flagged as ready once the headers of the final response have been
 * received.
 **/
static size_t
http_header_cb(char *ptr, size_t size, size_t nmemb, void *ctx) {
  http_response_t *resp = ctx;
  size_t len = size * nmemb;
  const char *sp;

  if(resp->ready) {
    return len;
  }

  if(len > 5 && !strncasecmp(ptr, "HTTP/", 5)) {
    http_headers_free(resp->headers);
    resp->headers = 0;
    resp->tail = 0;
    resp->status = 0;

    if((sp=memchr(ptr, ' ', len))) {
      resp->status = strtol(sp+1, 0, 10);
    }

    return len;
  }

  if(len && ptr[0] != '\r' && ptr[0] != '\n') {
    http_headers_append(resp, ptr, len);
    return len;
  }

  // an empty line terminates the header section
  if(resp->status >= 100 && resp->status < 200) {
    return len; // informational, the real response follows
  }

  if(resp->status >= 300 && resp->status < 400 &&
     http_response_header(resp, "Location")) {
    resp->skip = 1; // libcurl follows the redirect, drop this body
    return len;
  }

  resp->skip = 0;
  resp->ready = 1;

  return len;
}


/**
 * Buffer the body sent by a remote server.
 *
 * libcurl keeps whatever it could not hand over and offers it again once the
 * transfer is unpaused, so no data is lost when the buffer is full.
 **/
static size_t
http_write_cb(char *ptr, size_t size, size_t nmemb, void *ctx) {
  http_response_t *resp = ctx;
  size_t len = size * nmemb;

  if(resp->skip) {
    return len;
  }

  if(len > sizeof(resp->buf) - resp->len) {
    if(!resp->len) {
      return 0; // does not fit in an empty buffer, abort rather than stall
    }
    resp->paused = 1;
    return CURL_WRITEFUNC_PAUSE;
  }

  memcpy(resp->buf + resp->len, ptr, len);
  resp->len += len;

  return len;
}


/**
 * Reap the result of a finished transfer.
 **/
static void
http_response_finish(http_response_t *resp) {
  CURLMsg *msg;
  int left = 0;

  while((msg=curl_multi_info_read(resp->multi, &left))) {
    if(msg->msg != CURLMSG_DONE) {
      continue;
    }
    if(msg->data.result != CURLE_OK) {
      fprintf(stderr, "curl_multi_perform: %s\n",
	      curl_easy_strerror(msg->data.result));
      resp->failed = 1;
    }
  }

  resp->done = 1;
}


/**
 * Let libcurl make progress, waiting for socket activity if it has nothing
 * to hand over yet.
 **/
static int
http_response_perform(http_response_t *resp) {
  int running = 0;

  if(curl_multi_perform(resp->multi, &running) != CURLM_OK) {
    resp->failed = 1;
    resp->done = 1;
    return -1;
  }

  if(!running) {
    http_response_finish(resp);
    return 0;
  }

  if(resp->len || resp->paused) {
    return 0;
  }

#if LIBCURL_VERSION_NUM >= 0x074200 // 7.66.0
  if(curl_multi_poll(resp->multi, 0, 0, HTTP_POLL_TIMEOUT, 0) != CURLM_OK) {
#else
  if(curl_multi_wait(resp->multi, 0, 0, HTTP_POLL_TIMEOUT, 0) != CURLM_OK) {
#endif
    resp->failed = 1;
    resp->done = 1;
    return -1;
  }

  return 0;
}


http_response_t*
http_request(const char *method, const char *url,
	     const char * const *headers) {
  http_response_t *resp;
  struct curl_slist *sl;

  pthread_once(&g_curl_once, http_curl_init);

  if(!url || !url[0]) {
    return 0;
  }

  if(!method) {
    method = "GET";
  }

  if(!(resp=calloc(1, sizeof(http_response_t)))) {
    return 0;
  }

  if(!(resp->curl=curl_easy_init()) || !(resp->multi=curl_multi_init())) {
    http_response_free(resp);
    return 0;
  }

  for(; headers && *headers; headers++) {
    if(!(sl=curl_slist_append(resp->reqhdrs, *headers))) {
      break;
    }
    resp->reqhdrs = sl;
  }

  if(!strcasecmp(method, "HEAD")) {
    curl_easy_setopt(resp->curl, CURLOPT_NOBODY, 1l);
  } else if(strcasecmp(method, "GET")) {
    curl_easy_setopt(resp->curl, CURLOPT_CUSTOMREQUEST, method);
  }

  curl_easy_setopt(resp->curl, CURLOPT_URL, url);
  curl_easy_setopt(resp->curl, CURLOPT_HTTPHEADER, resp->reqhdrs);
  http_curl_restrict(resp->curl);
  curl_easy_setopt(resp->curl, CURLOPT_FOLLOWLOCATION, 1l);
  curl_easy_setopt(resp->curl, CURLOPT_MAXREDIRS, (long)HTTP_MAX_REDIRECTS);
  curl_easy_setopt(resp->curl, CURLOPT_NOSIGNAL, 1l);
  curl_easy_setopt(resp->curl, CURLOPT_BUFFERSIZE,
		   (long)HTTP_CURL_BUFFER_SIZE);
  curl_easy_setopt(resp->curl, CURLOPT_CONNECTTIMEOUT,
		   (long)HTTP_CONNECT_TIMEOUT);
  curl_easy_setopt(resp->curl, CURLOPT_LOW_SPEED_LIMIT,
		   (long)HTTP_STALL_LIMIT);
  curl_easy_setopt(resp->curl, CURLOPT_LOW_SPEED_TIME,
		   (long)HTTP_STALL_TIMEOUT);
  curl_easy_setopt(resp->curl, CURLOPT_HEADERFUNCTION, http_header_cb);
  curl_easy_setopt(resp->curl, CURLOPT_HEADERDATA, resp);
  curl_easy_setopt(resp->curl, CURLOPT_WRITEFUNCTION, http_write_cb);
  curl_easy_setopt(resp->curl, CURLOPT_WRITEDATA, resp);
#ifdef VERSION_TAG
  curl_easy_setopt(resp->curl, CURLOPT_USERAGENT, "websrv/" VERSION_TAG);
#endif

  if(curl_multi_add_handle(resp->multi, resp->curl) != CURLM_OK) {
    http_response_free(resp);
    return 0;
  }

  while(!resp->ready && !resp->done) {
    if(http_response_perform(resp)) {
      break;
    }
  }

  if(!resp->ready) {
    http_response_free(resp);
    return 0;
  }

  return resp;
}


ssize_t
http_response_read(http_response_t *resp, void *buf, size_t len) {
  size_t n;

  if(!resp || !buf || !len) {
    return -1;
  }

  while(resp->off >= resp->len) {
    resp->off = 0;
    resp->len = 0;

    if(resp->done) {
      return resp->failed ? -1 : 0;
    }

    if(resp->paused) {
      resp->paused = 0;
      if(curl_easy_pause(resp->curl, CURLPAUSE_CONT) != CURLE_OK) {
	resp->failed = 1;
	return -1;
      }
      continue;
    }

    if(http_response_perform(resp)) {
      return -1;
    }
  }

  if((n=resp->len - resp->off) > len) {
    n = len;
  }

  memcpy(buf, resp->buf + resp->off, n);
  resp->off += n;

  return (ssize_t)n;
}


long
http_response_status(const http_response_t *resp) {
  return resp ? resp->status : 0;
}


const char*
http_response_header(const http_response_t *resp, const char *name) {
  http_header_t *hdr;

  if(!resp) {
    return 0;
  }

  for(hdr=resp->headers; hdr; hdr=hdr->next) {
    if(!strcasecmp(hdr->name, name)) {
      return hdr->value;
    }
  }

  return 0;
}


void
http_response_foreach_header(const http_response_t *resp,
			     http_header_cb_t cb, void *ctx) {
  http_header_t *hdr;

  if(!resp || !cb) {
    return;
  }

  for(hdr=resp->headers; hdr; hdr=hdr->next) {
    cb(hdr->name, hdr->value, ctx);
  }
}


void
http_response_free(http_response_t *resp) {
  if(!resp) {
    return;
  }

  if(resp->multi) {
    if(resp->curl) {
      curl_multi_remove_handle(resp->multi, resp->curl);
    }
    curl_multi_cleanup(resp->multi);
  }
  if(resp->curl) {
    curl_easy_cleanup(resp->curl);
  }
  if(resp->reqhdrs) {
    curl_slist_free_all(resp->reqhdrs);
  }

  http_headers_free(resp->headers);
  free(resp);
}


static size_t
http_buffer_cb(char *ptr, size_t size, size_t nmemb, void *ctx) {
  http_buffer_t *buf = ctx;
  size_t len = size * nmemb;
  uint8_t *data;

  if(!(data=realloc(buf->data, buf->size + len + 1))) {
    return 0;
  }

  memcpy(data + buf->size, ptr, len);
  buf->data = data;
  buf->size += len;
  buf->data[buf->size] = 0;

  return len;
}


uint8_t*
http_get(const char *url, size_t *size) {
  http_buffer_t buf = {0, 0};
  long status = 0;
  CURLcode res;
  CURL *curl;

  pthread_once(&g_curl_once, http_curl_init);

  if(!url || !url[0]) {
    return 0;
  }

  if(!(curl=curl_easy_init())) {
    return 0;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);
  http_curl_restrict(curl);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1l);
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, (long)HTTP_MAX_REDIRECTS);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1l);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, (long)HTTP_CONNECT_TIMEOUT);
  curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, (long)HTTP_STALL_LIMIT);
  curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, (long)HTTP_STALL_TIMEOUT);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_buffer_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
#ifdef VERSION_TAG
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "websrv/" VERSION_TAG);
#endif

  res = curl_easy_perform(curl);
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
  curl_easy_cleanup(curl);

  if(res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform: %s\n", curl_easy_strerror(res));
    free(buf.data);
    return 0;
  }

  if(status >= 400) {
    fprintf(stderr, "http_get: %s responded with %ld\n", url, status);
    free(buf.data);
    return 0;
  }

  if(!buf.data) {
    buf.data = calloc(1, 1);
  }

  if(size) {
    *size = buf.size;
  }

  return buf.data;
}
