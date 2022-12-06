#!/bin/sh

rm -rf build/
mkdir build/
cd build/
cmake .. -DLLVM_DIR=/usr/lib/llvm-9/cmake
make
