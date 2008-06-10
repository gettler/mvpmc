#!/bin/sh

export ROOT="/opt/sybhttpd/localhost.drives/${SCRIPT_NAME%%etc*}"
export MVPMC_ROOT=$ROOT

export PATH="$ROOT/bin:$ROOT/usr/bin:$ROOT/sbin:$ROOT/usr/sbin:$PATH"
export PLUGIN_PATH="$ROOT/usr/share/mvpmc/plugins"
export LD_LIBRARY_PATH="$ROOT/lib"

LD_LIBRARY_PATH= /bin/sh "$ROOT/etc/web.sh" mvpmc

exit 0
