#!/bin/sh
#
# create a tarball containing the NMT filesystem
#

set -e

STRIP=${CROSS}strip
TOOLLIB=`dirname ${CROSS}`/../mipsel-linux-uclibc/lib
FS=filesystem/nmt
TOP=`pwd`

DIRS="bin sbin usr/bin usr/sbin lib dev proc var usr/share usr/share/mvpmc usr/share/udhcpc etc tmp oldroot usr/share/mvpmc/plugins"

BIN="busybox mvpmc osdtest strace"
SBIN="smartctl dropbearmulti dropbear dropbearkey fusermount"
TBIN="gdb gdbserver"
PLUGINS="libmvpmc_osd.plugin libmvpmc_html.plugin libmvpmc_http.plugin"
LIB="libinput.so libgw.so librefmem.so libosd.so"
TLIB="libc.so.0 libm.so.0 libcrypt.so.0 libpthread.so.0 libthread_db.so.1 libutil.so.0 libdl.so.0 libresolv.so.0"
LDLIB="ld-uClibc-0.9.28.so ld-uClibc.so.0 libdl-0.9.28.so"
GCCLIB="libgcc_s.so.1"

rm -rf $FS/install

for i in $DIRS ; do
    mkdir -p $FS/install/$i
done

for i in $PLUGINS ; do
    cp -d install/nmt/lib/$i $FS/install/usr/share/mvpmc/plugins
done

for i in $BIN ; do
    cp -d install/nmt/bin/$i $FS/install/bin
done

for i in $SBIN ; do
    cp -d install/nmt/sbin/$i $FS/install/sbin
done

for i in $TBIN ; do
    cp -d $TOOLLIB/../target_utils/bin/$i $FS/install/bin
done

for i in $LIB ; do
    cp -d install/nmt/lib/$i $FS/install/lib
done

for i in $LDLIB ; do
    cp -d $TOOLLIB/$i $FS/install/lib
done

for i in $TLIB ; do
    cp $TOOLLIB/$i $FS/install/lib
done

for i in $GCCLIB ; do
    cp $TOOLLIB/$i $FS/install/lib
done

awk -F/ '{if(/^\/bin\/[^\/]+$/) { system("ln -s busybox '$FS'/install" $0 ) } else {rp=sprintf("%" NF-2 "s", ""); gsub(/./,"../",rp); system("ln -sf " rp "bin/busybox '$FS'/install" $0) }}' apps/busybox/nmt/busybox-*/busybox.links

cd $FS/install
tar -cf $TOP/../nmtfs.tar *
cd $TOP/..
gzip nmtfs.tar
