#!/bin/sh

OUTPUT=/tmp/output

rm -f $OUPUT

echo "<html>" > $OUTPUT

start_telnet() {
    cat > /tmp/login <<EOF
#!/bin/sh
cd $ROOT
. /etc/profile
. etc/profile

export PATH=$ROOT/bin:$ROOT/usr/bin:$ROOT/sbin:$ROOT/usr/sbin:$PATH
export PLUGIN_PATH=$ROOT/usr/share/mvpmc/plugins
export LD_LIBRARY_PATH=$ROOT/lib

sh
EOF
    chmod 755 /tmp/login

    telnetd -l /tmp/login -p 23 &

    IP=`ifconfig eth0 | grep inet | cut -f2 -d: | cut -f1 -d' '`
    echo "<h1>Telnetd started...</h1>" >> $OUTPUT
    echo "<br><h2>IP: $IP</h2><br>" >> $OUTPUT
}

start_mvpmc() {
    sleep 3
    PID=`pidof gaya`
    if [ "$PID" != "" ] ; then
	kill $PID
	while [ "$PID" != "" ] ; do
	    sleep 1
	    PID=`pidof gaya`
	done
    fi
    sleep 2
    export LD_LIBRARY_PATH=$ROOT/lib
    export PLUGIN_PATH=$ROOT/usr/share/mvpmc/plugins
    mvpmc > $ROOT/mvpmc.out 2>&1
    exit 0
}

if [ "$1" = "telnet" ] ; then
    start_telnet
fi

if [ "$1" = "mvpmc" ] ; then
    PID=`pidof telnetd`
    if [ "$PID" = "" ] ; then
	start_telnet
    fi

    start_mvpmc &

    echo "<h1>mvpmc starting...</h1>" >> $OUTPUT
fi

echo "</html>" >> $OUTPUT

LEN=`wc -c $OUTPUT`

echo "Content-Length: $LEN";
echo "Content-type: text/html";
echo "";

cat $OUTPUT

exit 0
