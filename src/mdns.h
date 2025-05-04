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

#include <microhttpd.h>


/**
 * Start the mDNS service discovery.
 **/
int mdns_discovery_start(void);


/**
 * Stop the mDNS service discovery.
 **/
int mdns_discovery_stop(void);


/**
 * Respond to a mDNS discovery request.
 **/
enum MHD_Result mdns_request(struct MHD_Connection *conn,
                             const char* url);

