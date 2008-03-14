#!/bin/bash
#
# Build toolchain for NMT in ~/toolchains
#
# This is a slightly modified version of what is found here:
#   http://www.lundman.net/wiki/index.php/NMT:cross
#

set -x

TOP=`pwd`

if [ "$1" = "" ] ; then
    DOWNLOADS=`pwd`
else
    DOWNLOADS=$1
fi

for i in binutils-2.17.tar.gz \
         gcc-4.0.4.tar.gz \
         linux-2.6.15.7.tar.gz \
         uClibc-0.9.28.3.tar.bz2 ; do
    wget -c -O $DOWNLOADS/$i http://www.mvpmc.org/dl/$i
done

mkdir source
cd source

tar -xzf $DOWNLOADS/binutils-2.17.tar.gz
tar -xzf $DOWNLOADS/gcc-4.0.4.tar.gz
tar -xzf $DOWNLOADS/linux-2.6.15.7.tar.gz
tar -xjf $DOWNLOADS/uClibc-0.9.28.3.tar.bz2

cd gcc-4.04
patch -p1 < ../../gcc-uclibc-mips.patch
cd $TOP

export CTARGET=mipsel-linux-uclibc
export ARCH=mips

export TOOLCHAIN=$HOME/toolchains/

rm -rf build
mkdir build
cd build
$TOP/source/binutils-2.17/configure \
	--target=$CTARGET \
	--prefix=$TOOLCHAIN/$ARCH/ \
	--with-sysroot=$TOOLCHAIN/$ARCH/$CTARGET \
	--disable-werror
make
make install

# It installs things we don't want (and will fail later)
rm -rf $TOOLCHAIN/$ARCH/{info,lib,man,share}

export PATH=$TOOLCHAIN/$ARCH/bin:$PATH


cd $TOP/source/linux-2.6.15.7
yes "" | make ARCH=$ARCH oldconfig prepare

#With 2.6.x, this will probably end in an error because you don't have a gcc 
#cross-compiler yet, but you can ignore that.  Just copy over the headers:

mkdir -p $TOOLCHAIN/$ARCH/$CTARGET/usr/include/
rsync -arv include/linux include/asm-generic $TOOLCHAIN/$ARCH/$CTARGET/usr/include/
rsync -arv include/asm-$ARCH/ $TOOLCHAIN/$ARCH/$CTARGET/usr/include/asm
 
cd $TOP/source/uClibc-0.9.28.3/

cp $TOP/uclibc.config .config
make oldconfig
#make menuconfig
#Target Architecture: mips
#Target Architecture Features and Options: 
#  Target Process Architecture: MIPS I
#  Target Process Endianness: Little Endian
#  Target CPU has a memory management unit (MMU): YES
#  Enable floating point number support: YES
#  Target CPU has a floating point unit: NO
#  (/usr/local/mips/mipsel-linux-uclibc/usr) Linux kernel header location
#
#Library Installation Options
#  (/usr/local/mips/mipsel-linux-uclibc) uClibc runtime library directory
#  (/usr/local/mips/mipsel-linux-uclibc/usr) uClibc development environment
#
#uClibc development/debugging options
#  (mipsel-linux-uclibc-) Cross-compiling toolchain prefix

# This command should fail.
make CROSS=mipsel-linux-uclibc-  

# Copy the new include files
rsync -arvL include/ $TOOLCHAIN/$ARCH/$CTARGET/sys-include


cd $TOP/build/
rm -rf *

$TOP/source/gcc-4.0.4/configure \
	--target=$CTARGET \
	--prefix=$TOOLCHAIN/mips \
	--with-sysroot=$TOOLCHAIN/mips/$CTARGET \
	--enable-languages=c \
	--disable-shared \
	--disable-checking \
	--disable-werror \
       --disable-__cxa_atexit \
       --enable-target-optspace \
       --disable-nls \
       --enable-multilib \
       --with-float=soft \
       --enable-sjlj-exceptions \
       --disable-threads

make
make install

# Remove the temporary headers, and compile it properly.
rm -rf $TOOLCHAIN/$ARCH/$CTARGET/sys-include


cd $TOP/source/uClibc-0.9.28.3/

make CROSS=mipsel-linux-uclibc-
make install

# This bit fixes the C++ issue. But should not be required. TODO: Figure out why
cd $TOOLCHAIN/mips/$CTARGET/lib
ln -s ../usr/lib//crti.o .
ln -s ../usr/lib//crtn.o .
ln -s ../usr/lib//crt1.o .

# Now recompile gcc fully.
cd $TOP/build/
rm -rf *
$TOP/source/gcc-4.0.4/configure \
       --target=$CTARGET \
       --prefix=$TOOLCHAIN/mips \
       --with-sysroot=$TOOLCHAIN/mips/$CTARGET \
       --enable-languages=c,c++ \
       --enable-shared \
       --disable-checking \
       --disable-werror \
       --disable-__cxa_atexit \
       --enable-target-optspace \
       --disable-nls \
       --enable-multilib \
       --with-float=soft \
       --enable-sjlj-exceptions \
       --enable-threads=posix


# It is likely it will fail on __ctype_touplow_t*, this is quite easy to fix.
# http://gcc.gnu.org/bugzilla/show_bug.cgi?id=14939
# I went with the #ifdef changes of ../gcc-4.0.4/libstdc++-v3/config/locale/generic/c_locale.h
# and ../gcc-4.0.4/libstdc++-v3/config/os/gnu-linux/ctype_base.h 

make 
make install

exit 0
