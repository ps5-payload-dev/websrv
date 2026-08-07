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

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "ssdp.h"
#include "websrv.h"


/**
 * SSDP constants, see the UPnP Device Architecture specification.
 **/
#define SSDP_ADDR "239.255.255.250"
#define SSDP_PORT 1900
#define SSDP_TTL  4
#define SSDP_MX   3

/**
 * Search target used when probing the network. Devices respond once per
 * service they provide, e.g., upnp:rootdevice, uuid:<id>, and
 * urn:schemas-upnp-org:device:MediaServer:1.
 **/
#define SSDP_ST "ssdp:all"

/**
 * Number of seconds between two rounds of M-SEARCH requests.
 **/
#define SSDP_INTERVAL 30

/**
 * TTL used for responses that omit a CACHE-CONTROL header.
 **/
#define SSDP_DEFAULT_TTL 180


/**
 * Data structure used to keep track of services.
 **/
typedef struct service_seq {
  char usn[256];
  char st[256];
  char server[256];
  char location[512];
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
 * Check if the discovery thread should keep running.
 **/
static bool
ssdp_is_running(void) {
  bool running;

  pthread_mutex_lock(&g_lock);
  running = g_running;
  pthread_mutex_unlock(&g_lock);

  return running;
}


/**
 * Remove unresponsive services.
 **/
static void
ssdp_purge_services(void) {
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
 * Remove all known services.
 **/
static void
ssdp_flush_services(void) {
  service_seq_t* curr;
  service_seq_t* next;

  pthread_mutex_lock(&g_lock);

  curr = g_service_seq;
  g_service_seq = 0;

  pthread_mutex_unlock(&g_lock);

  while(curr) {
    next = curr->next;
    free(curr);
    curr = next;
  }
}


/**
 * Copy the value of an HTTP-style header into a buffer.
 **/
static int
ssdp_header_value(const char* resp, const char* name, char* buf, size_t size) {
  size_t namelen = strlen(name);
  const char* line;
  const char* end;
  const char* val;
  size_t linelen;
  size_t len;

  buf[0] = 0;

  for(line=resp; line && *line;) {
    end = strpbrk(line, "\r\n");
    linelen = end ? (size_t)(end - line) : strlen(line);

    if(linelen > namelen && !strncasecmp(line, name, namelen)) {
      val = line + namelen;

      while(*val == ' ' || *val == '\t') {
        val++;
      }

      if(*val == ':') {
        val++;
        while(*val == ' ' || *val == '\t') {
          val++;
        }

        if((size_t)(val - line) >= linelen) {
          return 0;
        }

        len = linelen - (size_t)(val - line);
        while(len && (val[len-1] == ' ' || val[len-1] == '\t')) {
          len--;
        }
        if(len >= size) {
          len = size - 1;
        }

        memcpy(buf, val, len);
        buf[len] = 0;

        return 0;
      }
    }

    if(!end) {
      break;
    }
    line = end + strspn(end, "\r\n");
  }

  return -1;
}


/**
 * Parse the max-age directive of a CACHE-CONTROL header.
 **/
static time_t
ssdp_cache_ttl(const char* resp) {
  char buf[128] = {0};
  const char* p;

  if(ssdp_header_value(resp, "CACHE-CONTROL", buf, sizeof(buf))) {
    return SSDP_DEFAULT_TTL;
  }

  for(p=buf; *p; p++) {
    if(strncasecmp(p, "max-age", 7)) {
      continue;
    }

    p += 7;
    while(*p == ' ' || *p == '\t') {
      p++;
    }
    if(*p != '=') {
      continue;
    }

    p++;
    while(*p == ' ' || *p == '\t') {
      p++;
    }

    if(atoi(p) > 0) {
      return (time_t)atoi(p);
    }
    break;
  }

  return SSDP_DEFAULT_TTL;
}


/**
 * Parse the port number of a URL
 **/
static uint16_t
ssdp_location_port(const char* url) {
  const char* p;

  if(!(p=strstr(url, "://"))) {
    return 0;
  }

  for(p+=3; *p && *p != '/' && *p != ':'; p++);

  if(*p == ':') {
    return (uint16_t)atoi(p + 1);
  }

  if(!strncmp("https://", url, 8)) {
    return 443;
  }

  return 80;
}


/**
 * Forget a service that announced its departure.
 **/
static void
ssdp_service_lost(const char* usn) {
  service_seq_t* ss;

  pthread_mutex_lock(&g_lock);

  for(ss=g_service_seq; ss; ss=ss->next) {
    if(!strcmp(ss->usn, usn)) {
      ss->ttl = 0;
      break;
    }
  }

  pthread_mutex_unlock(&g_lock);

  ssdp_purge_services();
}


/**
 * Remember a service announced via SSDP.
 **/
static void
ssdp_service_found(const char* resp, const char* addr) {
  service_seq_t* ss = 0;
  char location[512];
  char server[256];
  char usn[256];
  char nts[64];
  char st[256];
  time_t ttl;

  if(ssdp_header_value(resp, "USN", usn, sizeof(usn)) || !usn[0]) {
    return;
  }

  // NOTIFY messages use NT instead of ST, and NTS to signal their intent
  if(ssdp_header_value(resp, "ST", st, sizeof(st))) {
    ssdp_header_value(resp, "NT", st, sizeof(st));
  }
  if(!ssdp_header_value(resp, "NTS", nts, sizeof(nts))) {
    if(!strncasecmp(nts, "ssdp:byebye", 11)) {
      ssdp_service_lost(usn);
      return;
    }
  }

  if(ssdp_header_value(resp, "LOCATION", location, sizeof(location)) ||
     !location[0]) {
    return;
  }
  ssdp_header_value(resp, "SERVER", server, sizeof(server));

  ttl = ssdp_cache_ttl(resp);

  pthread_mutex_lock(&g_lock);

  for(ss=g_service_seq; ss; ss=ss->next) {
    if(!strcmp(ss->usn, usn)) {
      break;
    }
  }

  if(!ss) {
    if(!(ss=malloc(sizeof(service_seq_t)))) {
      pthread_mutex_unlock(&g_lock);
      return;
    }
    snprintf(ss->usn, sizeof(ss->usn), "%s", usn);
    ss->next = g_service_seq;
    g_service_seq = ss;
  }

  snprintf(ss->st, sizeof(ss->st), "%s", st);
  snprintf(ss->server, sizeof(ss->server), "%s", server);
  snprintf(ss->location, sizeof(ss->location), "%s", location);
  snprintf(ss->addr, sizeof(ss->addr), "%s", addr);
  ss->port = ssdp_location_port(location);
  ss->ttl = time(0) + ttl;

  pthread_mutex_unlock(&g_lock);
}


/**
 * Open a socket for SSDP traffic.
 **/
static int
ssdp_socket_open(void) {
  struct sockaddr_in sin;
  struct ip_mreq mreq;
  int ttl = SSDP_TTL;
  struct timeval tv;
  int reuse = 1;
  int fd;

  if((fd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    perror("socket");
    return -1;
  }

  if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    perror("setsockopt");
  }
  if(setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
    perror("setsockopt");
  }

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(SSDP_PORT);
  sin.sin_addr.s_addr = htonl(INADDR_ANY);

  if(bind(fd, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
    perror("bind");
  } else {
    memset(&mreq, 0, sizeof(mreq));
    mreq.imr_multiaddr.s_addr = inet_addr(SSDP_ADDR);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    if(setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
      perror("setsockopt");
    }
  }

  if(setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
    perror("setsockopt");
    close(fd);
    return -1;
  }

  tv.tv_sec = 1;
  tv.tv_usec = 0;
  if(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    perror("setsockopt");
    close(fd);
    return -1;
  }

  return fd;
}


/**
 * Multicast an M-SEARCH request.
 **/
static int
ssdp_send_msearch(int fd) {
  struct sockaddr_in sin;
  char req[512];
  int len;

  len = snprintf(req, sizeof(req),
                 "M-SEARCH * HTTP/1.1\r\n"
                 "HOST: " SSDP_ADDR ":%d\r\n"
                 "MAN: \"ssdp:discover\"\r\n"
                 "MX: %d\r\n"
                 "ST: " SSDP_ST "\r\n"
                 "\r\n", SSDP_PORT, SSDP_MX);

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(SSDP_PORT);
  sin.sin_addr.s_addr = inet_addr(SSDP_ADDR);

  if(sendto(fd, req, len, 0, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
    perror("sendto");
    return -1;
  }

  return 0;
}


/**
 * Thread for running SSDP service discovery.
 **/
static void*
ssdp_discovery_thread(void* args) {
  char addr[INET_ADDRSTRLEN];
  struct sockaddr_in sin;
  time_t deadline;
  char buf[4096];
  socklen_t len;
  ssize_t size;
  int fd;

  if((fd=ssdp_socket_open()) < 0) {
    return 0;
  }

  while(ssdp_is_running()) {
    ssdp_send_msearch(fd);

    deadline = time(0) + SSDP_INTERVAL;
    while(ssdp_is_running() && time(0) < deadline) {
      len = sizeof(sin);
      size = recvfrom(fd, buf, sizeof(buf) - 1, 0,
                      (struct sockaddr*)&sin, &len);
      if(size < 0) {
        if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
          continue;
        }
        perror("recvfrom");
        break;
      }

      buf[size] = 0;
      if(!inet_ntop(AF_INET, &sin.sin_addr, addr, sizeof(addr))) {
        continue;
      }

      ssdp_service_found(buf, addr);
    }

    ssdp_purge_services();
  }

  close(fd);

  ssdp_flush_services();

  pthread_mutex_lock(&g_lock);
  g_running = false;
  pthread_mutex_unlock(&g_lock);

  return 0;
}


int
ssdp_discovery_stop(void) {
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
ssdp_discovery_start(void) {
  bool start;

  pthread_mutex_lock(&g_lock);
  start = !g_running;
  g_running = true;
  pthread_mutex_unlock(&g_lock);

  if(!start) {
    return -1;
  }

  return pthread_create(&g_thread, 0, ssdp_discovery_thread, 0);
}


/**
 * Append a string to a JSON buffer.
 **/
static char*
ssdp_json_string(char* ptr, const char* str) {
  for(; *str; str++) {
    switch(*str) {
    case '"':
      ptr += sprintf(ptr, "\\\"");
      break;

    case '\\':
      ptr += sprintf(ptr, "\\\\");
      break;

    default:
      if((unsigned char)*str < 0x20) {
        ptr += sprintf(ptr, "\\u%04x", *str);
      } else {
        *ptr++ = *str;
      }
      break;
    }
  }

  *ptr = 0;

  return ptr;
}


enum MHD_Result
ssdp_request(struct MHD_Connection *conn, const char* url) {
  enum MHD_Result ret = MHD_NO;
  struct MHD_Response *resp;
  service_seq_t* ss;
  bool first = true;
  size_t size = 0;
  const char* st;
  char *buf;
  char *ptr;

  st = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "st");

  pthread_mutex_lock(&g_lock);

  // estimate needed memory, assume worst-case escaping
  for(ss=g_service_seq; ss; ss=ss->next) {
    size += (strlen(ss->usn) + strlen(ss->st) + strlen(ss->server) +
             strlen(ss->location) + strlen(ss->addr)) * 6;
    size += 128;
  }
  size += 16;

  if(!(buf=ptr=malloc(size))) {
    pthread_mutex_unlock(&g_lock);
    return ret;
  }

  ptr += sprintf(ptr, "[\n");
  for(ss=g_service_seq; ss; ss=ss->next) {
    if(st && strcmp(st, ss->st)) {
      continue;
    }
    if(!first) {
      ptr += sprintf(ptr, ",\n");
    }
    first = false;

    ptr += sprintf(ptr, "  {\"usn\":\"");
    ptr = ssdp_json_string(ptr, ss->usn);
    ptr += sprintf(ptr, "\",\"st\":\"");
    ptr = ssdp_json_string(ptr, ss->st);
    ptr += sprintf(ptr, "\",\"server\":\"");
    ptr = ssdp_json_string(ptr, ss->server);
    ptr += sprintf(ptr, "\",\"location\":\"");
    ptr = ssdp_json_string(ptr, ss->location);
    ptr += sprintf(ptr, "\",\"address\":\"");
    ptr = ssdp_json_string(ptr, ss->addr);
    ptr += sprintf(ptr, "\",\"port\": %d}", ss->port);
  }
  ptr += sprintf(ptr, "\n]\n");

  pthread_mutex_unlock(&g_lock);

  size = ptr - buf;
  if((resp=MHD_create_response_from_buffer(size, buf,
					   MHD_RESPMEM_MUST_FREE))) {
    MHD_add_response_header(resp, MHD_HTTP_HEADER_CONTENT_TYPE,
                            "application/json");
    ret = websrv_queue_response(conn, MHD_HTTP_OK, resp);
    MHD_destroy_response(resp);
  }

  return ret;
}
