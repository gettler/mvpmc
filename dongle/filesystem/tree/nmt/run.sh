#!/bin/sh

PID=`pidof gaya`
if [ "$PID" != "" ] ; then
    echo "Killing gaya"
    kill $PID
    while [ "$PID" != "" ] ; do
        sleep 1
        PID=`pidof gaya`
    done
fi

echo "Starting mvpmc"

ROOT=`pwd`

export PATH="$ROOT/bin:$ROOT/usr/bin:$ROOT/sbin:$ROOT/usr/sbin:$PATH"
export PLUGIN_PATH=${ROOT}/usr/share/mvpmc/plugins
export LD_LIBRARY_PATH=${ROOT}/lib

${ROOT}/bin/$@

