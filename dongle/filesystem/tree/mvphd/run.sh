#!/bin/bash

killall curacao

ROOT=`pwd`

export LD_LIBRARY_PATH=${ROOT}/lib
export PLUGIN_PATH=${ROOT}/usr/share/mvpmc/plugins

${ROOT}/bin/$@

