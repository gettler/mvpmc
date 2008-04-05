#!/bin/sh
#
# Copyright (C) 2004-2008, Jon Gettler
# http://www.mvpmc.org/
#
# This script will copy the needed files out of a kernel build workarea
# for use by the dongle_build.sh script.
#

WORKAREA=$1
TARGET=$2

BOOT=${WORKAREA}/arch/ppc/boot

SERIAL=${BOOT}/common/serial_stub.o
CFILES=${BOOT}/common/{dummy,misc-common,ns16550,relocate,string,util}.o
SFILES=${BOOT}/simple/{embed_config,head,misc-embedded}.o
KFILE=${BOOT}/images/vmlinux.gz

FILES=`sh -c "echo $CFILES $SFILES $KFILE ${BOOT}/lib/zlib.a ${BOOT}/ld.script"`

mkdir -p $TARGET

rm -f $TARGET/*.o
cp $FILES $TARGET

if [ -f $SERIAL ] ; then
    echo "KERNELVER=2.4.31" > ${TARGET}/version
    cp $SERIAL $TARGET
else
    echo "Unsupported kernel version"
    exit 1
fi
