#!/usr/bin/python
#
# SCons build script for mvpmc
# http://www.mvpmc.org/
#

import os
import sys

pwd = os.getcwd()
os.environ['TOPLEVEL'] = pwd
sys.path.append('%s/scons' % pwd)

import mvpmc

env = Environment (ENV = os.environ)

target = ARGUMENTS.get('TARGET', Platform())
kernver = ARGUMENTS.get('KERNVER', Platform())

env.Replace(CCFLAGS = '-O3 -g -Wall -Werror')
env.Replace(TOPDIR = os.getcwd())

home = os.environ['HOME']

#
# Allow the user to supply their own cross-compiler
#
if os.environ.has_key('CROSS'):
	cross = os.environ['CROSS']
	if os.path.exists(cross + 'gcc') == 0:
		print "cross-compiler '%s-gcc' not found"%cross
		sys.exit(1)
else:
	cross = ''

crosstool = '/opt/crosstool'
toolchains =  '/opt/toolchains/'

if os.path.exists(toolchains) == 0:
	toolchains =  home + '/toolchains/'

#
# parse the TARGET= option
#    mvp
#    mg35
#    nmt
#    host
#    kernel
#
if target == 'mvp':
	prefix = 'powerpc-linux-uclibc-'
	if cross == '':
		arch = 'powerpc-405-linux-uclibc'
		gcc = 'gcc-3.4.5-uClibc-0.9.28'
		prefix = arch + '-'
		crossroot = toolchains + '/' + arch + '/' + gcc + '/'
		cross = crossroot + '/bin/' + prefix
	cppflags = '-DMVPMC_MEDIAMVP'
	cc = cross + 'gcc'
	ldflags = ''
	env.Replace(LINKMODE = 'dynamic')
	env.Replace(GTKCFLAGS = '')
	env.Replace(GTKLDFLAGS = '')
elif target == 'mg35':
	# DSLinux toolchain is at http://dslinux.org/wiki/Compiling_DSLinux
	if cross == '':
		crossroot = '/usr/local/dslinux-toolchain-2006-11-04-i686'
		cross = crossroot + '/bin/' + 'arm-linux-elf-'
		cc = cross + 'gcc'
	cppflags = '-DMVPMC_MG35'
	ldflags = '-Wl,-elf2flt'
	env.Replace(LINKMODE = 'static')
	env.Replace(GTKCFLAGS = '')
	env.Replace(GTKLDFLAGS = '')
	if os.path.exists(cc) == 0:
		print "ARM cross-compiler '%s' not found"%cc
		print "Please see http://dslinux.org/wiki/Compiling_DSLinux"
		sys.exit(1)
elif target == 'nmt':
	if cross == '':
		arch = 'mips'
		prefix = 'mipsel-linux-uclibc-'
		gcc = ''
		crossroot = toolchains + '/mips/' + gcc + '/'
		cross = crossroot + '/bin/' + prefix
	cppflags = '-DMVPMC_NMT'
	cc = cross + 'gcc'
	ldflags = ''
	env.Replace(LINKMODE = 'dynamic')
	env.Replace(GTKCFLAGS = '')
	env.Replace(GTKLDFLAGS = '')
elif target == 'host':
	cppflags = '-DMVPMC_HOST'
	crossroot = ''
	ldflags = ''
	env.Replace(LINKMODE = 'dynamic')
	fd = os.popen('pkg-config --cflags gtk+-2.0 gthread-2.0')
	gtkcflags = fd.readline()[:-2]
	fd.close()
	fd = os.popen('pkg-config --libs gtk+-2.0 gthread-2.0')
	gtkldflags = fd.readline()[:-2]
	fd.close()
	env.Replace(GTKCFLAGS = gtkcflags)
	env.Replace(GTKLDFLAGS = gtkldflags)
	env.Replace(CROSS = '')
elif target == 'kernel':
	print "kernel build"
	arch = 'powerpc-405-linux-uclibc'
	gcc = 'gcc-3.4.5-uClibc-0.9.28'
	crossroot = toolchains + '/' + arch + '/' + gcc + '/'
	prefix = arch + '-'
	cross = crossroot + '/bin/' + prefix
	cc = cross + 'gcc'
	cppflags = ''
else:
	print "Unknown target %s"%target
	sys.exit(1)

#
# Rebuilding the cross-compiler should be done in ~/toolchains
#
if (target != 'host') and (os.path.exists(cc) == 0):
	toolchains =  home + '/toolchains/'
	crossroot = toolchains + '/' + arch + '/' + gcc + '/'
	cross = crossroot + '/bin/' + prefix
	cc = cross + 'gcc'

#
# build binaries in the obj/TARGET directory
#
env.Replace(INCDIR = pwd + '/include')
env.Replace(INSTDIR = pwd + '/dongle/install/' + target)
env.Replace(INSTINCDIR = pwd + '/dongle/install/' + target + '/include')
env.Replace(INSTLIBDIR = pwd + '/dongle/install/' + target + '/lib')
env.Replace(INSTBINDIR = pwd + '/dongle/install/' + target + '/bin')
env.Replace(INSTSHAREDIR = pwd + '/dongle/install/' + target + '/usr/share/mvpmc')
env.Replace(BUILD_DIR = 'obj/' + target)
env.Replace(TARG = target)
env.Replace(DOWNLOADS = home + '/downloads')
env.Replace(TOPLEVEL = pwd)
env.Replace(CPPFLAGS = cppflags)
env.Replace(LDFLAGS = ldflags)

env.Replace(CROSS = cross)
env.Replace(CC = cross + 'gcc')

crossroot = os.path.dirname(cross)
env.Replace(CROSSPATH = crossroot)
Export('env','crossroot')

#
# Dynamically loadable plugins are only avaiable with dynamic linking
#
linkmode = env['LINKMODE']
if linkmode == 'static':
	env.Append(CCFLAGS = ' -DLINKMODE_STATIC')
if linkmode == 'dynamic':
	env.Append(CCFLAGS = ' -DLINKMODE_DYNAMIC')
	env.Append(CCFLAGS = ' -DPLUGIN_SUPPORT')

#
# ensure the download directory exits
#
downloads = env['DOWNLOADS']
if os.path.exists(downloads) == 0:
	os.mkdir(downloads)

if target == 'kernel':
	#
	# do the kernel build
	#
	cc = env['CC']
	kern = env.SConscript('dongle/kernel/linux-2.4.31/SConscript')
	gccbuild = 'tools/toolchains/uclibc/SConscript'
	if os.path.exists(cc) == 0:
		print "build kernel cross-compiler"
		gcc = env.SConscript(gccbuild)
		env.Depends(kern, gcc)
elif target == 'mg35':
	dir = env['BUILD_DIR']
	libs = env.SConscript('dongle/libs/SConscript')
	mvplibs = env.SConscript('libs/SConscript')
	plugins = env.SConscript('plugins/SConscript')
	mvpmc = env.SConscript('src/SConscript',
			       build_dir='src/'+dir, duplicate=0)
	inc = env.SConscript('include/SConscript')
	env.Depends(mvplibs, inc)
	env.Depends(mvplibs, libs)
	env.Depends(plugins, mvplibs)
	env.Depends(mvpmc, plugins)
	env.Depends(mvpmc, mvplibs)
	env.Depends(mvpmc, libs)
elif target == 'nmt':
	dir = env['BUILD_DIR']
	libs = env.SConscript('dongle/libs/SConscript')
	mvplibs = env.SConscript('libs/SConscript')
	plugins = env.SConscript('plugins/SConscript')
	mvpmc = env.SConscript('src/SConscript',
			       build_dir='src/'+dir, duplicate=0)
	cc = env['CC']
	if os.path.exists(cc) == 0:
		print "build application cross-compiler"
		gcc = env.SConscript('tools/toolchains/uclibc/SConscript')
		env.Depends(libs, gcc)
		env.Depends(mvplibs, gcc)
		env.Depends(mvpmc, gcc)
	inc = env.SConscript('include/SConscript')
	apps = env.SConscript('dongle/apps/SConscript')
	env.Depends(mvplibs, inc)
	env.Depends(mvplibs, libs)
	env.Depends(plugins, mvplibs)
	env.Depends(mvpmc, plugins)
	env.Depends(mvpmc, mvplibs)
	env.Depends(mvpmc, libs)
	dongle = env.SConscript('dongle/SConscript')
	env.Depends(dongle, mvpmc)
	env.Depends(dongle, apps)
else:
	#
	# do the application build
	#
	inc = env.SConscript('include/SConscript')
	dir = env['BUILD_DIR']

	#
	# Only build the apps for the mvp.
	#
	if target == 'mvp':
		apps = env.SConscript('dongle/apps/SConscript')

	libs = env.SConscript('dongle/libs/SConscript')
	plugins = env.SConscript('plugins/SConscript')
	mvplibs = env.SConscript('libs/SConscript')
	mvpmc = env.SConscript('src/SConscript',
			       build_dir='src/'+dir, duplicate=0)
	themes = env.SConscript('themes/SConscript')
	images = env.SConscript('images/SConscript')

	env.Depends(mvpmc, libs)
	env.Depends(mvplibs, libs)

	#
	# Install the cross compilation tools, if needed.
	#
	if target == 'mvp':
		cc = env['CC']
		if os.path.exists(cc) == 0:
			print "build application cross-compiler"
			gcc = env.SConscript('tools/toolchains/uclibc/SConscript')
			env.Depends(libs, gcc)
			env.Depends(apps, gcc)
			env.Depends(mvplibs, gcc)
			env.Depends(mvpmc, gcc)

	#
	# Build the dongle.bin file
	#
	if target == 'mvp':
		dongle = env.SConscript('dongle/SConscript')
		env.Depends(dongle, mvpmc)
		env.Depends(dongle, mvplibs)
		env.Depends(dongle, apps)
		env.Depends(dongle, libs)
		env.Depends(dongle, themes)
		env.Depends(dongle, images)

	#
	# Build squashfs and mktree and mvprelay
	#
	if target == 'mvp':
		cc = env['CC']
		squashfs = env.SConscript('tools/squashfs/SConscript')
		mktree = env.SConscript('tools/mktree/SConscript')
		mvprelay = env.SConscript('tools/mvprelay/SConscript')
		env.Depends(dongle, squashfs)
		env.Depends(dongle, mktree)

	#
	# Misc tools
	#
	if target == 'mvp':
		misc = env.SConscript('tools/misc/SConscript')
		passwd = env.SConscript('tools/dongle_passwd/SConscript')

	#
	# Try and ensure a valid build order (is this really needed?)
	#
	env.Depends(mvplibs, libs)
	env.Depends(plugins, mvplibs)
	for a in libs:
	    env.Depends(mvpmc, a)
	env.Depends(mvpmc, mvplibs)
	env.Depends(mvpmc, plugins)
