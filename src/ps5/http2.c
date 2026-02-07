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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


typedef struct http2_ctx {
  int libnetMemId;
  int libsslCtxId;
  int libhttpCtxId;
  int tmplId;
  int reqId;
} http2_ctx_t;


int sceNetPoolCreate(const char*, int, int);
int sceNetPoolDestroy(int);

int sceSslInit(size_t);
int sceSslTerm(int);

int sceHttp2Init(int, int, size_t, int);
int sceHttp2Term(int);

int sceHttp2CreateTemplate(int, const char*, int, int);
int sceHttp2SetSslCallback(int, void*, void*);
int sceHttp2DeleteTemplate(int);

int sceHttp2CreateRequestWithURL(int, const char*, const char*, uint64_t);
int sceHttp2DeleteRequest(int);

int sceHttp2GetResponseContentLength(int, int*, uint64_t*);
int sceHttp2SendRequest(int, const void*, size_t);
int sceHttp2GetStatusCode(int, int*);
int sceHttp2ReadData(int, void *, size_t);


/**
 *
 **/
static int
http2_ssl_cb(void) {
  return 0;
}


/**
 *
 **/
static int
http2_init(http2_ctx_t* ctx, const char* agent, const char* method,
	   const char* url) {
  int err;

  ctx->libnetMemId  = -1;
  ctx->libsslCtxId  = -1;
  ctx->libhttpCtxId = -1;
  ctx->tmplId       = -1;
  ctx->reqId        = -1;

  if((ctx->libnetMemId=sceNetPoolCreate("http2_get", 32*1024, 0)) < 0) {
    return ctx->libnetMemId;
  }

  if((ctx->libsslCtxId=sceSslInit(256*1024)) < 0) {
    return ctx->libsslCtxId;
  }

  if((ctx->libhttpCtxId=sceHttp2Init(ctx->libnetMemId, ctx->libsslCtxId,
				     256*1024, 1)) < 0) {
    return ctx->libhttpCtxId;
  }

  if((ctx->tmplId=sceHttp2CreateTemplate(ctx->libhttpCtxId, agent, 3, 1)) < 0) {
    return ctx->tmplId;
  }

  if((err=sceHttp2SetSslCallback(ctx->tmplId, http2_ssl_cb, 0))) {
    return err;
  }

  if((ctx->reqId=sceHttp2CreateRequestWithURL(ctx->tmplId, method, url, 0)) < 0) {
    return ctx->reqId;
  }

  return 0;
}


/**
 *
 **/
static int
http2_request(http2_ctx_t* ctx, uint8_t** data, size_t* len) {
  int status = -1;
  int err;
  int n;

  if((err=sceHttp2SendRequest(ctx->reqId, 0, 0))) {
    return err;
  }
  if((err=sceHttp2GetStatusCode(ctx->reqId, &status))) {
    return err;
  }
  if((err=sceHttp2GetResponseContentLength(ctx->reqId, &err, len))) {
    return err;
  }

  if(!len || status != 200) {
    // TODO: support resources without a content length
    return status;
  }

  if(!(*data=malloc(*len))) {
    return -1;
  }

  if((n=sceHttp2ReadData(ctx->reqId, *data, *len) < 0)) {
    return n;
  }

  return status;
}


/**
 *
 **/
static void
http2_fini(http2_ctx_t* ctx) {
  if(ctx->reqId >= 0) {
    sceHttp2DeleteRequest(ctx->reqId);
  }
  if(ctx->tmplId >= 0) {
    sceHttp2DeleteTemplate(ctx->tmplId);
  }
  if(ctx->libhttpCtxId >= 0) {
    sceHttp2Term(ctx->libhttpCtxId);
  }
  if(ctx->libsslCtxId >= 0) {
    sceSslTerm(ctx->libsslCtxId);
  }
  if(ctx->libnetMemId >= 0) {
    sceNetPoolDestroy(ctx->libnetMemId);
  }
}


uint8_t*
http2_get(const char* url, size_t* len) {
  http2_ctx_t ctx;
  size_t size = 0;
  uint8_t* data;

  if((http2_init(&ctx, "websrv/"VERSION_TAG, "GET", url))) {
    http2_fini(&ctx);
    return 0;
  }

  if(http2_request(&ctx, &data, &size) != 200) {
    http2_fini(&ctx);
    return 0;
  }

  http2_fini(&ctx);;
  if(len) {
    *len = size;
  }
  return data;
}
