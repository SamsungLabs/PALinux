#!/bin/sh

KERNEL=$1
DTB=$2
ROOTFS=$3

sudo python3 ./m1/script/proxyclient/linux.py -b 'earlycon console=ttySAC0,1500000 console=tty0 debug' $KERNEL $DTB $ROOTFS
