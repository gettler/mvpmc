#!/bin/bash
#
# create a tarball containing the MediaMVP-HD filesystem
#

set -e

STRIP=${CROSS}strip
TOOLLIB=`dirname ${CROSS}`/../mipsel-linux-uclibc/lib
FS=filesystem/mvphd
TOP=`pwd`

DIRS="bin sbin usr/bin usr/sbin lib dev proc var usr/share usr/share/mvpmc usr/share/udhcpc etc tmp oldroot usr/share/mvpmc/plugins"

BIN="busybox mvpmc osdtest strace mythfuse"
SBIN="smartctl dropbearmulti dropbear dropbearkey fusermount"
TBIN="gdb gdbserver"
PLUGINS="libmvpmc_osd.plugin libmvpmc_html.plugin libmvpmc_http.plugin libmvpmc_av.plugin libmvpmc_screensaver.plugin libmvpmc_myth.plugin"
LIB="libinput.so libgw.so librefmem.so libosd.so libcurl.so libcurl.so.4 libcurl.so.4.0.1 libcmyth.so libfuse.so libfuse.so.2 libfuse.so.2.7.2"
TLIB="libc.so.0 libm.so.0 libcrypt.so.0 libpthread.so.0 libthread_db.so.1 libutil.so.0 libdl.so.0 libresolv.so.0"
LDLIB="ld-uClibc-0.9.28.so ld-uClibc.so.0 libdl-0.9.28.so"
GCCLIB="libgcc_s.so.1"
WEB="web.css"

rm -rf $FS/install

for i in $DIRS ; do
    mkdir -p $FS/install/$i
done

cd filesystem/tree/mvphd
tar -cf - * | tar -xf - -C ../../mvphd/install
cd $TOP

for i in $PLUGINS ; do
    cp -d install/mvphd/lib/$i $FS/install/usr/share/mvpmc/plugins
done

for i in $WEB ; do
    cp -d install/mvphd/usr/share/mvpmc/$i $FS/install/usr/share/mvpmc
done

for i in $BIN ; do
    cp -d install/mvphd/bin/$i $FS/install/bin
done

for i in $SBIN ; do
    cp -d install/mvphd/sbin/$i $FS/install/sbin
done

for i in $TBIN ; do
    cp -d $TOOLLIB/../target_utils/bin/$i $FS/install/bin
done

for i in $LIB ; do
    cp -d install/mvphd/lib/$i $FS/install/lib
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

awk -F/ '{if(/^\/bin\/[^\/]+$/) { system("ln -s busybox '$FS'/install" $0 ) } else {rp=sprintf("%" NF-2 "s", ""); gsub(/./,"../",rp); system("ln -sf " rp "bin/busybox '$FS'/install" $0) }}' apps/busybox/mvphd/busybox-*/busybox.links

cd $FS/install
tar -cf $TOP/../mvphdfs.tar *
cd $TOP/..
gzip -f mvphdfs.tar
