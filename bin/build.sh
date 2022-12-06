#!/bin/sh

# GCC
./bin/build-gcc.sh

# GCC plugin
make build-xcfi

# LLVM plugin
cd analyzer/analyzer-tool
./build.sh
