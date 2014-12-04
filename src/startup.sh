#!/bin/sh -e

IPC_PATH=${IPC_PATH:-/tmp/hfwm.sock}

hc() {
	echo -n $* | socat -t0 unix-sendto:$IPC_PATH stdin
}

randcol() {
	dd if=/dev/urandom bs=1 count=1 2>/dev/null \
	| od -t u4 \
	| awk 'NR==1 { print $2 }' \
	| ( read n; echo $(( 4 + $n % 2 )) )
}

hc spawn xsetroot -solid '#'`randcol``randcol``randcol`
hc bind b spawn randbg # script for same as above

hc bind x spawn xclock
hc bind Mod4-x spawn xeyes

#exit 0

#hc bind a spawn xterm
hc bind a spawn xlogo
hc bind z spawn xclock
hc bind Button1 spawn xterm
hc bind Button3 spawn xterm

hc bind q split next vert
hc bind w split next horiz
hc bind e split prev vert
hc bind r split prev horiz
hc bind t merge prev
hc bind y merge next
hc bind u redist prev 10
hc bind i redist next 10

hc bind h focus sibling next
hc bind j focus sibling prev
hc bind k focus lineage next
hc bind l focus lineage prev
hc bind c layout next

