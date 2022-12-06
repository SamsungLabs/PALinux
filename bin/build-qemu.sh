#!/bin/bash

ROOT=$(realpath $(dirname "$0"))/..
QEMU=$ROOT/qemu
BUILD=$QEMU/build

mkdir -p $BUILD
cd $BUILD

if [[ ! -e Makefile ]]; then
  ../configure --target-list=aarch64-softmmu,aarch64-linux-user
fi

make -j $(ncpus)

