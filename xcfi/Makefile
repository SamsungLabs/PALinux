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
PLUGIN_NAME = xcfi.so

PLUGIN_DIR = gcc-plugin
TEST_DIR = test
PACBENCH_DIR = pacbench
OUT_DIR= out

TEST_TOOLCHAIN_PREFIX=aarch64-linux-gnu-

.PHONY: all clean check build-test clean-test plugin pacbench

all: check

plugin:
	$(MAKE) -C ${PLUGIN_DIR} OUTPUT=${PLUGIN_NAME} all
	mkdir -p ${OUT_DIR}
	mv ${PLUGIN_DIR}/${PLUGIN_NAME} ${OUT_DIR}/.

check: plugin
	$(CXX) -fplugin=${OUT_DIR}/${PLUGIN_NAME} -c -x c++ /dev/null -o /dev/null

pacbench:
	$(MAKE) -C ${PACBENCH_DIR} KDIR=../${KDIR} all
	mv ${PACBENCH_DIR}/*.ko ${OUT_DIR}/.

test: test-nocfi test-cfi

test-nocfi: check clean-test
	$(MAKE) -C ${TEST_DIR} clean-objs
	$(MAKE) -C ${TEST_DIR} TOOLCHAIN_PREFIX=${TEST_TOOLCHAIN_PREFIX} all
	mkdir -p ${OUT_DIR}
	mv ${TEST_DIR}/test-* ${TEST_DIR}/example-* ${OUT_DIR}/.

test-cfi: check clean-test
	$(MAKE) -C ${TEST_DIR} clean-objs
	$(MAKE) -C ${TEST_DIR} TOOLCHAIN_PREFIX=${TEST_TOOLCHAIN_PREFIX} PLUGIN=../${OUT_DIR}/${PLUGIN_NAME} all
	mkdir -p ${OUT_DIR}
	mv ${TEST_DIR}/test-* ${TEST_DIR}/example-* ${OUT_DIR}/.

clean: clean-plugin clean-test clean-pacbench

clean-plugin:
	$(MAKE) -C ${PLUGIN_DIR} TARGET=${PLUGIN_TARGET} clean
	rm -f out/${PLUGIN_NAME}

clean-test:
	$(MAKE) -C ${TEST_DIR} clean
	rm -f out/test*

clean-pacbench:
	$(MAKE) -C ${PACBENCH_DIR} clean
	rm -f out/*.ko
