#!/bin/sh -e

IPC_PATH=${IPC_PATH:-/tmp/hfwm.sock}

hc() {
	echo -n $* | socat -t0 unix-sendto:$IPC_PATH stdin
}

rand() {
	dd if=/dev/urandom bs=1 count=1 2>/dev/null \
	| od -t u4 \
	| awk 'NR==1 { print $2 }' \
	| ( read n; echo $(( 4 + $n % 2 )) )
}

hc spawn xsetroot -solid '#'`rand``rand``rand`

