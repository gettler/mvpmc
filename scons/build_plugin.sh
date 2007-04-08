#!/bin/sh

help() {
    exit 0
}

#set -x

ARCHIVES=
TARGET=

while getopts "a:ht:" i
  do case "$i" in
      a) ARCHIVES="$ARCHIVES $OPTARG" ;;
      h) help ;;
      t) TARGET=$OPTARG ;;
      *) echo error ; exit 1 ;;
  esac
done

if [ "$ARCHIVES" = "" ] ; then
    echo "No archives specified!"
    exit 1
fi

if [ "$TARGET" = "" ] ; then
    echo "No target specified!"
    exit 1
fi

DIR=/tmp/mvpmc_pi_$RANDOM
WDIR=`pwd`

mkdir $DIR
cd $DIR

N=0
for i in $ARCHIVES ; do
    cp $i $DIR
    cd $DIR
    NAME=`basename $i`
    ar x $NAME
    for j in `ls *.o | grep -v ^lib` ; do
	mv $j ${NAME}_${j}
    done
    cd $WDIR
    let N=N+1
done

ar r $TARGET $DIR/*.o

rm -rf $DIR

exit 0
