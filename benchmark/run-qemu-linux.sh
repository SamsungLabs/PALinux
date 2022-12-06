#!/bin/bash

ROOT=$(realpath $(dirname "$0"))/..
QEMU=${QEMU:-$ROOT/out/qemu-system-aarch64}
KERN=$1
INIT=$2
ARGS=(-M virt
      -cpu max
      -smp 4
      -m 2048
      -nographic)

$QEMU "${ARGS[@]}" -kernel $KERN -initrd $INIT -append "root=/dev/ram rdinit=/etc/init.d/rcS" #"$@"
