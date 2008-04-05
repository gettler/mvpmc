#
# MediaMVP Media Center (mvpmc)
# http://www.mvpmc.org/
#

all: mvp nmt mg35 host

mvp:
	scons -Q -j 2 TARGET=mvp

host:
	scons -Q -j 2 TARGET=host

mg35:
	scons -Q -j 2 TARGET=mg35

nmt:
	scons -Q -j 2 TARGET=nmt

kernel:
	scons -Q TARGET=kernel

kernel_31: kernel

docs:
	doxygen Doxyfile

cscope:
	find src libs include -name \*.c -or -name \*.h > cscope.files
	cscope -b -q -k

mvp_clean:
	scons -c TARGET=mvp
	rm -f dongle.bin.mvpmc.ver
	rm -rf dongle/install/mvp
	rm -rf `find dongle/apps -name mvp -type d`
	rm -rf `find dongle/libs -name mvp -type d`

host_clean:
	scons -c TARGET=host
	rm -rf dongle/install/host
	rm -rf `find dongle/apps -name host -type d`
	rm -rf `find dongle/libs -name host -type d`

mg35_clean:
	scons -c TARGET=mg35
	rm -rf dongle/install/mg35
	rm -rf `find dongle/apps -name mg35 -type d`
	rm -rf `find dongle/libs -name mg35 -type d`

nmt_clean:
	scons -c TARGET=nmt
	rm -rf dongle/install/nmt
	rm -rf `find dongle/apps -name nmt -type d`
	rm -rf `find dongle/libs -name nmt -type d`

clean: mvp_clean host_clean mg35_clean nmt_clean
	rm -rf `find libs -name obj -type d`
	rm -rf `find src -name obj -type d`
	rm -rf `find plugins -name obj -type d`

distclean: clean
	rm -rf dongle/install
	rm -rf dongle/filesystem/install
	rm -rf dongle/filesystem/install_wrapper
	rm -rf dongle/filesystem/mvp
	rm -rf dongle/filesystem/nmt
	rm -rf dongle/kernel/filesystem
	rm -rf dongle/kernel/linux-2.4.31/linux-2.4.31
	rm -rf dongle/kernel/linux-2.4.31/unionfs-1.0.14
	rm -rf dongle/kernel/linux-2.4.31/fuse/fuse-2.5.3
	rm -rf tools/toolchains/glibc/crosstool-0.42
	rm -rf tools/toolchains/uclibc/crosstool-0.28-rc5
	rm -rf tools/toolchains/uclibc/build
	rm -rf tools/toolchains/uclibc/source
	rm -rf tools/genext2fs/genext2fs-1.4rc1
	rm -rf tools/squashfs/squashfs2.2-r2
	rm -rf `find . -name .sconsign -type f`
	rm -rf home
	rm -rf doc/html
	rm -rf doc/latex
	rm -rf cscope*
	rm -f scons/WGet.pyc
	rm -f src/version.c
