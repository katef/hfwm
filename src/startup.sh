#!/bin/sh -e

IPC_PATH=${IPC_PATH:-/tmp/hfwm.sock}

hc() {
	echo $* | socat -t0 unix-sendto:$IPC_PATH stdin
}

hc hello
hc spawn xterm


