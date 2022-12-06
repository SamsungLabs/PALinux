#!/bin/bash

ROOT=$(realpath $(dirname "$0"))/..

mkdir -p $ROOT/gcc/build

cd $ROOT/gcc/build

# reconfigure only when Makefile is not there
if [[ ! -e Makefile ]]; then
  ../configure -v --with-pkgversion='Ubuntu/Linaro 7.4.0-1ubuntu1~18.04.1' --with-bugurl=file:///usr/share/doc/gcc-7/README.Bugs --enable-languages=c,c++ --prefix=/usr --with-gcc-major-version-only --program-suffix=-kpac-7 --enable-shared --enable-linker-build-id --libexecdir=/usr/lib --without-included-gettext --enable-threads=posix --libdir=/usr/lib --enable-nls --with-sysroot=/ --enable-clocale=gnu --enable-libstdcxx-debug --enable-libstdcxx-time=yes --with-default-libstdcxx-abi=new --enable-gnu-unique-object --disable-libquadmath --disable-libquadmath-support --enable-plugin --enable-default-pie --with-system-zlib --enable-multiarch --enable-fix-cortex-a53-843419 --disable-werror --enable-checking=release --build=x86_64-linux-gnu --host=x86_64-linux-gnu --target=aarch64-linux-gnu --program-prefix=aarch64-linux-gnu- --includedir=/usr/aarch64-linux-gnu/include
fi

make -j $(ncpus)

sudo make install
