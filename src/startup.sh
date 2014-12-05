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

MOD=Mod4

#hc unbind x
hc unbind

hc spawn xsetroot -solid '#'`randcol``randcol``randcol`
hc bind $MOD-b spawn randbg # script for same as above

hc bind $MOD-Shift-x       spawn xeyes
hc bind $MOD-Ctrl-x        spawn xlogo
hc bind $MOD-Shift-Ctrl-x  spawn xclock
hc bind $MOD-x             spawn xcalc

#exit 0

#hc bind $MOD-a spawn xterm
hc bind $MOD-a spawn xlogo
hc bind $MOD-z spawn xclock
hc bind $MOD-Button1 spawn xeyes +shape
hc bind $MOD-Button2 spawn xeyes +shape
hc bind $MOD-Button3 spawn xeyes +shape
hc bind $MOD-Button4 spawn xeyes +shape
hc bind $MOD-Button5 spawn xeyes +shape

#exit 0;

hc bind $MOD-q split next vert
hc bind $MOD-w split next horiz
hc bind $MOD-e split prev vert
hc bind $MOD-r split prev horiz
hc bind $MOD-t merge prev
hc bind $MOD-y merge next
hc bind $MOD-u redist prev 10
hc bind $MOD-i redist next 10

hc bind $MOD-j focus prev sibling
hc bind $MOD-h focus next sibling
hc bind $MOD-l focus prev lineage
hc bind $MOD-k focus next lineage
hc bind $MOD-f focus prev client
hc bind $MOD-g focus next client

hc bind $MOD-c layout next

