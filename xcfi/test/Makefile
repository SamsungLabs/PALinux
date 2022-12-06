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

CXX = ${TOOLCHAIN_PREFIX}g++

VTABLE_OBJS = test/class/a.o \
				test/class/b.o \
				test/class/c.o \
				test/class/class_world.o
ICALL_OBJS = test/callback/cb_a.o \
				test/callback/callback_world.o \
				test/callback/empty.o

OBJS = ${VTABLE_OBJS} ${ICALL_OBJS} test/main.o

CXXFLAGS = -std=c++11 -Wall -Wno-literal-suffix -flto
LDFLAGS = -static

# For debugging
LDFLAGS += -fdump-tree-all -fdump-ipa-all -fdump-rtl-all -fdump-class-hierarchy
CXXFLAGS += -fdump-tree-all -fdump-ipa-all -fdump-rtl-all -fdump-class-hierarchy

ifdef PLUGIN
PLUGINFLAGS += -fplugin=${PLUGIN} -march=armv8.3-a -msign-return-address=all
OUTPUT_POSTFIX = cfi
else
PLUGINFLAGS += -DNO_PLUGIN
OUTPUT_POSTFIX = nocfi
endif

LDFLAGS += ${PLUGINFLAGS}
CXXFLAGS += ${PLUGINFLAGS}

# for LKM
ifeq (${TOOLCHAIN_PREFIX}, aarch64-linux-gnu-)
ARCH=arm64
endif
PWD := $(shell pwd)
obj-m := example.o
example-objs := example/2.o example/kernel_main.o

.PHONY: all clean example

# top level goal: build our plugin as a shared library
all: test-$(OUTPUT_POSTFIX) example-$(OUTPUT_POSTFIX)
	mkdir -p dump-${OUTPUT_POSTFIX}
	mv `find . -maxdepth 1 -name "*.o.*"` dump-${OUTPUT_POSTFIX}/.
	mv `find test/. -name "*.cpp.*"` dump-${OUTPUT_POSTFIX}/.

test-${OUTPUT_POSTFIX} : $(OBJS)
	$(CXX) $(LDFLAGS) -fPIC -x lto -o $@ $^

%.o : %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

example-$(OUTPUT_POSTFIX) :
	$(CXX) $(PLUGINFLAGS) -static -fno-stack-protector \
					-o example-1-${OUTPUT_POSTFIX} \
					example/1.cpp example/user_main.cpp
	$(CXX) $(PLUGINFLAGS) -static -fno-stack-protector \
					-o example-2-${OUTPUT_POSTFIX} \
					example/2.c example/user_main.cpp
	$(CXX) $(PLUGINFLAGS) -static -fno-stack-protector \
					-o example-3-${OUTPUT_POSTFIX} \
					example/3.c
	$(CXX) $(PLUGINFLAGS) -static -fno-stack-protector \
					-o example-4-${OUTPUT_POSTFIX} \
					example/4.c example/user_main.cpp

lkm-example :
	make ARCH=${ARCH} CROSS_COMPILE=${TOOLCHAIN_PREFIX} -j $(ncpus) -C $(KDIR) KBUILD_EXTMOD=${PWD} modules

clean: clean-objs clean-lkm
	rm -rf test-* example-* dump-*

clean-objs:
	rm -rf ${OBJS}

clean-lkm:
	rm -rf *.o *.ko *.mod *.cmd .tmp_versions *.symvers *.order *.mod.c *.swp
