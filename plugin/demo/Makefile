#   Copyright (C) 2025 John Törnblom
#
# This file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

ifdef PS5_PAYLOAD_SDK
    include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
else
    CC ?= gcc
endif

WEBSRV_SRC := ../../src
CFLAGS += -I$(WEBSRV_SRC) -fPIC -shared -Wall -Werror
PLUGIN := demo.so

all: $(PLUGIN)

install: $(PLUGIN)
	mkdir -p ../../plugin/demo
	@if ! cmp -s $(PLUGIN) ../../plugin/demo/$(PLUGIN) 2>/dev/null; then \
	  cp -f $(PLUGIN) ../../plugin/demo/; \
	fi

$(PLUGIN): demo.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(PLUGIN)

.PHONY: all install clean
