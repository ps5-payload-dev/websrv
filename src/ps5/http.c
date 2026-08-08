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

#include "http.h"


/**
 * Memory pools used by libnet, libssl and libhttp. They are created once and
 * shared by all requests, so they must accommodate the transfers running in
 * parallel. Requests fail with SCE_HTTP_ERROR_OUT_OF_MEMORY (0x80431022)
 * when they are too small.
 **/
#define HTTP_NET_POOL_SIZE  (64  * 1024)
#define HTTP_SSL_POOL_SIZE  (256 * 1024)
#define HTTP_POOL_SIZE      (128 * 1024)


/**
 * Give up if the remote server does not resolve or accept the connection in
 * time (in seconds).
 **/
#define HTTP_RESOLVE_TIMEOUT 15
#define HTTP_CONNECT_TIMEOUT 15


/**
 * Give up if the remote server transfers nothing during HTTP_STALL_TIMEOUT
 * seconds.
 **/
#define HTTP_STALL_TIMEOUT   60


/**
 * Maximum size of the header section sent by a remote server.
 **/
#define HTTP_HEADER_MAX_SIZE 0x4000


/**
 * Maximum number of bytes pulled out of libhttp in one go, which reports the
 * number of bytes read in an int.
 **/
#define HTTP_READ_MAX        0x100000


/**
 * Amount of body data buffered by http_get() before its buffer is grown.
 **/
#define HTTP_BUFFER_BLOCK    0x10000


/**
 * Constants defined by libhttp.
 **/
#define SCE_HTTP_VERSION_1_1      2

#define SCE_HTTP_METHOD_GET       0
#define SCE_HTTP_METHOD_POST      1
#define SCE_HTTP_METHOD_HEAD      2
#define SCE_HTTP_METHOD_OPTIONS   3
#define SCE_HTTP_METHOD_PUT       4
#define SCE_HTTP_METHOD_DELETE    5
#define SCE_HTTP_METHOD_TRACE     6
#define SCE_HTTP_METHOD_CONNECT   7

#define SCE_HTTP_HEADER_OVERWRITE 0
#define SCE_HTTP_HEADER_ADD       1


#ifdef VERSION_TAG
#define HTTP_USER_AGENT "websrv/" VERSION_TAG
#else
#define HTTP_USER_AGENT "websrv"
#endif


int sceNetInit(void);
int sceNetPoolCreate(const char*, int, int);
int sceNetPoolDestroy(int);

int sceSslInit(size_t);
int sceSslTerm(int);

int sceHttpInit(int, int, size_t);
int sceHttpTerm(int);

int sceHttpCreateTemplate(int, const char*, int, int);
int sceHttpDeleteTemplate(int);
int sceHttpsSetSslCallback(int, void*, void*);
int sceHttpSetResponseHeaderMaxSize(int, size_t);
int sceHttpSetAutoRedirect(int, int);
int sceHttpSetResolveTimeOut(int, uint32_t);
int sceHttpSetConnectTimeOut(int, uint32_t);
int sceHttpSetSendTimeOut(int, uint32_t);
int sceHttpSetRecvTimeOut(int, uint32_t);

int sceHttpCreateConnectionWithURL(int, const char*, int);
int sceHttpDeleteConnection(int);

int sceHttpCreateRequestWithURL(int, int, const char*, uint64_t);
int sceHttpAddRequestHeader(int, const char*, const char*, uint32_t);
int sceHttpSendRequest(int, const void*, size_t);
int sceHttpGetStatusCode(int, int*);
int sceHttpGetAllResponseHeaders(int, char**, size_t*);
int sceHttpReadData(int, void*, size_t);
int sceHttpAbortRequest(int);
int sceHttpDeleteRequest(int);


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
 * libhttp buffers the transfer internally and hands over the body as it is
 * pulled out with http_response_read(), so a response only keeps track of
 * the objects that must be released, and of the headers that were copied out
 * of libhttp while the request was still alive.
 **/
struct http_response {
  int            tmplId;
  int            connId;
  int            reqId;

  long           status;
  http_header_t *headers;
  http_header_t *tail;

  int            eof;    // the whole body has been read
  int            failed; // transfer did not deliver a complete response
};


/**
 * libnet, libssl and libhttp are initialized on demand, and stay initialized
 * for the lifetime of the process.
 **/
static pthread_once_t g_http_once = PTHREAD_ONCE_INIT;

static int g_libnetMemId  = -1;
static int g_libsslCtxId  = -1;
static int g_libhttpCtxId = -1;


static void
http_perror(const char *fn, int err) {
  fprintf(stderr, "%s: 0x%08x\n", fn, (unsigned int)err);
}


/**
 * Accept whatever certificate the remote server presents, which is what the
 * libcurl-based implementation does with CURLOPT_SSL_VERIFYPEER.
 **/
static int
http_ssl_cb(int libsslCtxId, unsigned int verifyErr, void *const sslCert[],
	    int certNum, void *userArg) {
  return 0;
}


/**
 * Set up the pools shared by all requests.
 **/
static void
http_global_init(void) {
  // libnet may already have been initialized by another part of the payload
  sceNetInit();

  if((g_libnetMemId=sceNetPoolCreate("websrv", HTTP_NET_POOL_SIZE, 0)) < 0) {
    http_perror("sceNetPoolCreate", g_libnetMemId);
    g_libnetMemId = -1;
    return;
  }

  if((g_libsslCtxId=sceSslInit(HTTP_SSL_POOL_SIZE)) < 0) {
    http_perror("sceSslInit", g_libsslCtxId);
    sceNetPoolDestroy(g_libnetMemId);
    g_libnetMemId = -1;
    g_libsslCtxId = -1;
    return;
  }

  if((g_libhttpCtxId=sceHttpInit(g_libnetMemId, g_libsslCtxId,
				 HTTP_POOL_SIZE)) < 0) {
    http_perror("sceHttpInit", g_libhttpCtxId);
    sceSslTerm(g_libsslCtxId);
    sceNetPoolDestroy(g_libnetMemId);
    g_libnetMemId  = -1;
    g_libsslCtxId  = -1;
    g_libhttpCtxId = -1;
  }
}


/**
 * Only let libhttp speak http, both for the request itself and for any
 * redirect it follows. Without this, a URL like file:///etc/passwd would be
 * served by the proxy.
 **/
static int
http_url_supported(const char *url) {
  return url && (!strncasecmp(url, "http://", 7) ||
		 !strncasecmp(url, "https://", 8));
}


/**
 * Translate a method to the constant expected by libhttp, or -1 if libhttp
 * cannot issue it.
 **/
static int
http_method_id(const char *method) {
  if(!method || !strcasecmp(method, "GET")) {
    return SCE_HTTP_METHOD_GET;
  }
  if(!strcasecmp(method, "HEAD")) {
    return SCE_HTTP_METHOD_HEAD;
  }
  if(!strcasecmp(method, "POST")) {
    return SCE_HTTP_METHOD_POST;
  }
  if(!strcasecmp(method, "PUT")) {
    return SCE_HTTP_METHOD_PUT;
  }
  if(!strcasecmp(method, "DELETE")) {
    return SCE_HTTP_METHOD_DELETE;
  }
  if(!strcasecmp(method, "OPTIONS")) {
    return SCE_HTTP_METHOD_OPTIONS;
  }
  if(!strcasecmp(method, "TRACE")) {
    return SCE_HTTP_METHOD_TRACE;
  }
  if(!strcasecmp(method, "CONNECT")) {
    return SCE_HTTP_METHOD_CONNECT;
  }

  return -1;
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
 * libhttp hands over the header section of the final response, status line
 * included, in a buffer owned by the request, so the interesting parts are
 * copied out before the request is deleted. Continuation lines are dropped,
 * they have been deprecated for a long time.
 **/
static void
http_headers_parse(http_response_t *resp, const char *buf, size_t size) {
  const char *nl;
  size_t len;

  while(size) {
    if((nl=memchr(buf, '\n', size))) {
      len = nl - buf + 1;
    } else {
      len = size;
    }

    if(buf[0] != ' ' && buf[0] != '\t' &&
       !(len > 5 && !strncasecmp(buf, "HTTP/", 5))) {
      http_headers_append(resp, buf, len);
    }

    buf  += len;
    size -= len;
  }
}


http_response_t*
http_request(const char *method, const char *url,
	     const char * const *headers) {
  http_response_t *resp;
  int status = 0;
  char name[256];
  const char *val;
  char *hdrbuf = 0;
  size_t hdrlen = 0;
  size_t len;
  int id;
  int err;

  pthread_once(&g_http_once, http_global_init);

  if(g_libhttpCtxId < 0) {
    return 0;
  }

  if(!http_url_supported(url)) {
    return 0;
  }

  if((id=http_method_id(method)) < 0) {
    return 0;
  }

  if(!(resp=calloc(1, sizeof(http_response_t)))) {
    return 0;
  }

  resp->tmplId = -1;
  resp->connId = -1;
  resp->reqId  = -1;

  if((resp->tmplId=sceHttpCreateTemplate(g_libhttpCtxId, HTTP_USER_AGENT,
					 SCE_HTTP_VERSION_1_1, 1)) < 0) {
    http_perror("sceHttpCreateTemplate", resp->tmplId);
    resp->tmplId = -1;
    http_response_free(resp);
    return 0;
  }

  if((err=sceHttpsSetSslCallback(resp->tmplId, http_ssl_cb, 0))) {
    http_perror("sceHttpsSetSslCallback", err);
    http_response_free(resp);
    return 0;
  }

  if((err=sceHttpSetResponseHeaderMaxSize(resp->tmplId,
					  HTTP_HEADER_MAX_SIZE))) {
    http_perror("sceHttpSetResponseHeaderMaxSize", err);
    http_response_free(resp);
    return 0;
  }

  sceHttpSetAutoRedirect(resp->tmplId, 1);
  sceHttpSetResolveTimeOut(resp->tmplId, HTTP_RESOLVE_TIMEOUT * 1000000);
  sceHttpSetConnectTimeOut(resp->tmplId, HTTP_CONNECT_TIMEOUT * 1000000);
  sceHttpSetSendTimeOut(resp->tmplId, HTTP_STALL_TIMEOUT * 1000000);
  sceHttpSetRecvTimeOut(resp->tmplId, HTTP_STALL_TIMEOUT * 1000000);

  if((resp->connId=sceHttpCreateConnectionWithURL(resp->tmplId, url, 0)) < 0) {
    http_perror("sceHttpCreateConnectionWithURL", resp->connId);
    resp->connId = -1;
    http_response_free(resp);
    return 0;
  }

  if((resp->reqId=sceHttpCreateRequestWithURL(resp->connId, id, url, 0)) < 0) {
    http_perror("sceHttpCreateRequestWithURL", resp->reqId);
    resp->reqId = -1;
    http_response_free(resp);
    return 0;
  }

  for(; headers && *headers; headers++) {
    if(!(val=strchr(*headers, ':'))) {
      continue;
    }

    len = val - *headers;
    while(len && ((*headers)[len-1] == ' ' || (*headers)[len-1] == '\t')) {
      len--;
    }
    if(!len || len >= sizeof(name)) {
      continue;
    }

    memcpy(name, *headers, len);
    name[len] = 0;

    val++;
    while(*val == ' ' || *val == '\t') {
      val++;
    }

    if((err=sceHttpAddRequestHeader(resp->reqId, name, val,
				    SCE_HTTP_HEADER_ADD))) {
      http_perror("sceHttpAddRequestHeader", err);
    }
  }

  // blocks until the headers of the final response have been received
  if((err=sceHttpSendRequest(resp->reqId, 0, 0))) {
    http_perror("sceHttpSendRequest", err);
    http_response_free(resp);
    return 0;
  }

  if((err=sceHttpGetStatusCode(resp->reqId, &status))) {
    http_perror("sceHttpGetStatusCode", err);
    http_response_free(resp);
    return 0;
  }

  resp->status = status;

  if((err=sceHttpGetAllResponseHeaders(resp->reqId, &hdrbuf, &hdrlen))) {
    http_perror("sceHttpGetAllResponseHeaders", err);
  } else if(hdrbuf && hdrlen) {
    http_headers_parse(resp, hdrbuf, hdrlen);
  }

  // responses that never carry a body
  if(id == SCE_HTTP_METHOD_HEAD || status == 204 || status == 304 ||
     (status >= 100 && status < 200)) {
    resp->eof = 1;
  }

  return resp;
}


ssize_t
http_response_read(http_response_t *resp, void *buf, size_t len) {
  int n;

  if(!resp || !buf || !len) {
    return -1;
  }

  if(resp->eof) {
    return resp->failed ? -1 : 0;
  }

  if(len > HTTP_READ_MAX) {
    len = HTTP_READ_MAX;
  }

  if((n=sceHttpReadData(resp->reqId, buf, len)) < 0) {
    http_perror("sceHttpReadData", n);
    resp->failed = 1;
    resp->eof = 1;
    return -1;
  }

  if(!n) {
    resp->eof = 1;
  }

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

  if(resp->reqId >= 0) {
    if(!resp->eof) {
      // the caller gave up before the whole body was received
      sceHttpAbortRequest(resp->reqId);
    }
    sceHttpDeleteRequest(resp->reqId);
  }
  if(resp->connId >= 0) {
    sceHttpDeleteConnection(resp->connId);
  }
  if(resp->tmplId >= 0) {
    sceHttpDeleteTemplate(resp->tmplId);
  }

  http_headers_free(resp->headers);
  free(resp);
}


uint8_t*
http_get(const char *url, size_t *size) {
  http_response_t *resp;
  uint8_t *data = 0;
  size_t cap = 0;
  size_t len = 0;
  uint8_t *tmp;
  ssize_t n;

  if(!(resp=http_request("GET", url, 0))) {
    return 0;
  }

  if(http_response_status(resp) >= 400) {
    fprintf(stderr, "http_get: %s responded with %ld\n", url,
	    http_response_status(resp));
    http_response_free(resp);
    return 0;
  }

  while(1) {
    if(len + HTTP_BUFFER_BLOCK + 1 > cap) {
      cap = cap ? cap * 2 : 2 * HTTP_BUFFER_BLOCK;
      if(!(tmp=realloc(data, cap))) {
	free(data);
	http_response_free(resp);
	return 0;
      }
      data = tmp;
    }

    if((n=http_response_read(resp, data + len, HTTP_BUFFER_BLOCK)) < 0) {
      free(data);
      http_response_free(resp);
      return 0;
    }

    if(!n) {
      break;
    }

    len += n;
  }

  http_response_free(resp);

  data[len] = 0;

  if(size) {
    *size = len;
  }

  return data;
}
