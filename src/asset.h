/* Copyright (C) 2024 John Törnblom

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
 * Respond to an asset request.
 **/
enum MHD_Result asset_request(struct MHD_Connection *conn,
			      const char* url);

/**
 * Register the presence of an asset.
 **/
void asset_register(const char* path, void* data, size_t size, const char* mime);

