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
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <microdns/microdns.h>

#include "mdns.h"
#include "websrv.h"


/**
 * Data structure used to keep track of services.
 **/
typedef struct service_seq {
  char domain[256];
  char target[256];
  char prot[256];
  char addr[INET_ADDRSTRLEN];
  uint16_t port;
  time_t ttl;
  struct service_seq* next;
} service_seq_t;


/**
 * Global state variables.
 **/
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_t g_thread;
static bool g_running = false;
static service_seq_t* g_service_seq = 0;


/**
 * Remove unresponsive services.
 **/
static void
mdns_purge_services(void) {
  service_seq_t* prev = 0;
  service_seq_t* curr = 0;

  pthread_mutex_lock(&g_lock);

  curr = g_service_seq;
  while(curr) {
    if(curr->ttl > time(0)) {
      prev = curr;
      curr = curr->next;
    } else if(prev) {
      prev->next = curr->next;
      free(curr);
      curr = prev->next;
    } else {
      g_service_seq = curr->next;
      free(curr);
      curr = g_service_seq;
    }
  }

  pthread_mutex_unlock(&g_lock);
}


/**
 * Callback function used by libmicrodns to determine when to stop listening
 * for mDNS traffic.
 **/
static bool
mdns_stop_cb(void *ctx) {
  bool stop;

  pthread_mutex_lock(&g_lock);
  stop = !g_running;
  pthread_mutex_unlock(&g_lock);

  return stop;
}


/**
 * Callback function used my libmicrodns when services are discovered.
 **/
static void
mdns_discovery_cb(void *stc, int status, const struct rr_entry *entries) {
  const char* domain = 0;
  const char* target = 0;
  const char* addr = 0;
  const char* prot = 0;
  uint16_t port = 0;
  uint32_t ttl = 0;
  service_seq_t* ss;
  char err[128];

  if(status < 0) {
    mdns_strerror(status, err, sizeof(err));
    fprintf(stderr, "error: %s\n", err);
    return;
  }

  for(const struct rr_entry *e=entries; e; e=e->next) {
    switch(e->type) {
    case RR_A:
      addr = e->data.A.addr_str;
      ttl = e->ttl;
      break;

    case RR_SRV:
      target = e->data.SRV.target;
      port = e->data.SRV.port;
      break;

    case RR_PTR:
      domain = e->data.PTR.domain;
      prot = e->name;
      break;

    case RR_TXT:
      break;

    default:
      continue;
    }
  }

  if(!domain || !prot || !addr || !port || !target) {
    return;
  }

  mdns_purge_services();

  pthread_mutex_lock(&g_lock);

  for(ss=g_service_seq; ss; ss=ss->next) {
    if(!strncmp(ss->domain, domain, sizeof(ss->domain))) {
      break;
    }
  }

  if(!ss) {
    if((ss=malloc(sizeof(service_seq_t)))) {
      strncpy(ss->domain, domain, sizeof(ss->domain));
      ss->next = g_service_seq;
      g_service_seq = ss;
    }
  }

  strncpy(ss->target, target, sizeof(ss->target));
  strncpy(ss->prot, prot, sizeof(ss->prot));
  strncpy(ss->addr, addr, sizeof(ss->addr));
  ss->port = port;
  ss->ttl = time(0) + ttl;

  pthread_mutex_unlock(&g_lock);
}


/**
 * Thread for running mDNS service discovery.
 **/
static void*
mdns_discovery_thread(void* args) {
  const char *names[] = {
    "_smb._tcp.local",
    "_nfs._tcp.local"
  };
  size_t nb_names = 2;
  struct mdns_ctx *ctx;
  char err[128];
  int r;

  if((r=mdns_init(&ctx, MDNS_ADDR_IPV4, MDNS_PORT)) < 0) {
    mdns_strerror(r, err, sizeof(err));
    fprintf(stderr, "mdns_init: %s\n", err);

  } else if((r=mdns_listen(ctx, names, nb_names, RR_PTR, 30,
                           mdns_stop_cb, mdns_discovery_cb, 0)) < 0) {
    mdns_strerror(r, err, sizeof(err));
    fprintf(stderr, "mdns_listen: %s\n", err);
  }

  mdns_destroy(ctx);

  pthread_mutex_lock(&g_lock);
  g_running = false;
  pthread_mutex_unlock(&g_lock);

  return 0;
}


int
mdns_discovery_stop(void) {
  bool stop;

  pthread_mutex_lock(&g_lock);
  stop = g_running;
  g_running = false;
  pthread_mutex_unlock(&g_lock);

  if(!stop) {
    return -1;
  }

  return pthread_join(g_thread, 0);
}


int
mdns_discovery_start(void) {
  bool start;

  pthread_mutex_lock(&g_lock);
  start = !g_running;
  g_running = true;
  pthread_mutex_unlock(&g_lock);

  if(!start) {
    return -1;
  }

  return pthread_create(&g_thread, 0, mdns_discovery_thread, 0);
}


enum MHD_Result
mdns_request(struct MHD_Connection *conn, const char* url) {
  enum MHD_Result ret = MHD_NO;
  struct MHD_Response *resp;
  service_seq_t* ss;
  size_t size = 0;
  size_t cnt = 0;
  char *buf;
  char *ptr;

  pthread_mutex_lock(&g_lock);

  // esimate needed memory
  for(ss=g_service_seq; ss; ss=ss->next) {
    cnt++;
  }
  size = cnt*(sizeof(service_seq_t) + 100);
  size += 100;

  if(!(buf=ptr=malloc(size))) {
    return ret;
  }

  ptr += sprintf(buf, "[\n");
  for(ss=g_service_seq; ss; ss=ss->next) {
    if(ss != g_service_seq) {
      ptr += sprintf(ptr, ",\n");
    }
    ptr += sprintf(ptr, "  {\"domain\":\"%s\",", ss->domain);
    ptr += sprintf(ptr, "\"protocol\":\"%s\",", ss->prot);
    ptr += sprintf(ptr, "\"hostname\":\"%s\",", ss->target);
    ptr += sprintf(ptr, "\"address\":\"%s\",", ss->addr);
    ptr += sprintf(ptr, "\"port\": %d}", ss->port);
  }
  ptr += sprintf(ptr, "\n]\n");

  pthread_mutex_unlock(&g_lock);

  size = ptr-buf;
  if((resp=MHD_create_response_from_buffer(size, buf,
					   MHD_RESPMEM_MUST_FREE))) {
    MHD_add_response_header(resp, "Content-Type", "application/json");
    ret = websrv_queue_response(conn, MHD_HTTP_OK, resp);
    MHD_destroy_response(resp);
  }

  return ret;
}

