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

#include "plugin_api.h"


void plugin_load_all(void);

void plugin_unload_all(void);

/**
 * Dispatch @p url to a loaded plugin when it matches a registered prefix.
 * @return MHD_YES if a plugin handled the request, MHD_NO otherwise.
 */
enum MHD_Result plugin_request(struct MHD_Connection* conn,
			       const char* url,
			       const char* method,
			       const plugin_post_data_t* post);

/**
 * Respond to GET /plugin with an HTML list of loaded plugins.
 */
enum MHD_Result plugin_list_request(struct MHD_Connection* conn);
