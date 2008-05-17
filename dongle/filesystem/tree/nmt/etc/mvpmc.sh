#!/bin/sh

OUTPUT=/tmp/output.html

TOP=/opt/sybhttpd/localhost.drives

MOUNT="${SCRIPT_NAME%%mvpmc*}"

cat > $OUTPUT <<EOF
<h1>mvpmc</h1>
<hr>
<h2>Select an option:</h2>
<ul>
<li><a href="http://localhost.drives:8883${MOUNT}mvpmc/etc/script-telnet.cgi">Start telnetd</a></li>
<li><a href="http://localhost.drives:8883${MOUNT}mvpmc/etc/script-mvpmc.cgi">Start mvpmc</a></li>
</ul>
EOF

LEN=`wc -c $OUTPUT`

echo "Content-Length: $LEN";
echo "Content-type: text/html";
echo "";

cat $OUTPUT

exit 0
