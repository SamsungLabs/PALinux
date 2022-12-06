#
# Copyright (C) 2019 Samsung Electronics Co., Ltd All Rights Reserved
#
# Author: Sungbae Yoo <sungbae.yoo@samsung.com>
#
# This is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 3, or (at your option) any later
# version.
#
# This is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License at
# <http://www.gnu.org/licenses/>.
#

ifndef OUTPUT
OUTPUT = xcfi.so
endif

PASSES_OBJS = pass/armv8_3/vtv.o \
				pass/armv8_3/icv.o \
				pass/armv8_3/icv-wrap.o \
				pass/armv8_3/icv-context.o \
				pass/armv8_3/dump.o
PASSES_OBJS += pass/generic/vtv.o pass/generic/icv.o
PASSES_OBJS += pass/vtv.o pass/icv.o

UTIL_OBJS = util/gimple.o util/aarch64.o
OBJS = ${PASSES_OBJS} ${UTIL_OBJS} preference.o main.o

# -fno-rtti is required for GCC plugins
CXXFLAGS = -std=c++11 -Wall -fno-rtti
# Workaround for an issue of -std=c++11 and the current GCC headers
CXXFLAGS += -Wno-literal-suffix

# Determine the plugin-dir and add it to the flags
PLUGINDIR=$(shell $(CXX) -print-file-name=plugin)
CXXFLAGS += -I$(PLUGINDIR)/include -I.

.PHONY: all clean

# top level goal: build our plugin as a shared library
all: $(OUTPUT)

${OUTPUT}: $(OBJS)
	$(CXX) $(LDFLAGS) -shared -o $@ $^

%.o : %.cc
	$(CXX) $(CXXFLAGS) -fPIC -c -o $@ $<

clean:
	rm -f $(OUTPUT) $(OBJS)
