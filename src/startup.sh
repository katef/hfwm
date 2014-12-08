#!/bin/sh -e

# An example for setup and event automation for hfwm.
#
# This is as close as it gets to a configuration file for hfwm. There's no
# particular reason for this to be written as a shell script; if you want to
# do something more complex, then use whatever language seems suitable.
#
# Communication with hfwm is by IPC, over SOCK_DGRAM sockets in the AF_LOCAL.
# This script sets up a few things and then listens for incoming events.
# These things could be done by seperate scripts if you prefer; I've just put
# everything here so that it's easier to see what's involved.
#
# Things this script depends on:
#
#   socat    - for IPC. If you use a language with sockets, you won't need it.
#   xcompmgr - for rendering transparency
#   transset - for changing transparency per window
#   hsetroot - it's a bit nicer than xsetroot
#

if [ ! -S "$HFWM_IPC" ]; then
	echo '$HFWM_IPC required' >&2
	exit 1
fi

HFWM_SUB="`mktemp -up /tmp .hfsub-XXXXXX`"

hc() {
	echo -n $* | socat -t0 unix-sendto:"$HFWM_IPC" stdin
}

randcol() {
	dd if=/dev/urandom bs=1 count=1 2>/dev/null \
	| od -t u4 \
	| awk 'NR==1 { print $2 }' \
	| ( read n; echo $(( 4 + $n % 2 )) )
}

# This is usually the diamond key for a Sun, the command key for a mac, and the
# windows key for a PC. The idea here is that all window manager stuff strictly
# only uses this modifier, leaving everything else for applications.
#
# To show your current bindings: xmodmap -pm
MOD=Mod4

hc unbind

{
	sed 's/#.*$//' \
	| while read key cmd; do
		if [ -z $key ]; then
			continue
		fi

		# The idea here is to be consistent about using shift to mean
		# "the same thing, but backwards"
		if { echo "$cmd" | grep -q next; }; then
			prev="`echo $cmd | sed s/next/prev/g`"
			hc bind Shift-$MOD-$key $prev
		fi

		hc bind $MOD-$key $cmd

	done
} <<EOF
	Shift-x       spawn xeyes +shape
	Ctrl-x        spawn xlogo
	Shift-Ctrl-x  spawn xclock
	x             spawn xcalc
	f             spawn xfd -fn fixed

	a spawn xterm
	a spawn xlogo
	z spawn xclock

	# Mouse buttons are considered keys.
	# You can use modifiers with them, too.
	Button1 spawn xclock -hd red
	Button2 spawn xclock -hd green
	Button3 spawn xclock -hd blue
	Button4 spawn xclock -hd purple
	Button5 spawn xclock -hd yellow

	q split next vert
	w split next horiz

	t merge next

	u redist next 10

	h focus next sibling
	k focus next lineage
	g focus next client

	grave layout next
EOF

#pkill xcompmgr
#xcompmgr -n &

#hc spawn xsetroot -solid '#'`randcol``randcol``randcol`

trap "echo would rm -f $HFWM_SUB" INT QUIT

socat UNIX-RECV:$HFWM_SUB stdout \
| {
	hc subscribe $HFWM_SUB
	while read event args; do
		echo event=$event, args=$args

		case $event in
		enter)
			echo $args | {
				read target id

				case $target in
				root)
					;;

				frame)
					hc focusid $id frame
					;;

				client)
					hc focusid $id client
					;;

				unmanaged)
					;;
				esac
			}
			;;

		leave)
			echo $args | {
				read target id
			}
			;;

		focus)
			echo $args | {
				read target id

				case $target in
				frame)
					;;

				client)
					transset -i $id --inc > /dev/null
					;;
				esac
			}
			;;

		blur)
			echo $args | {
				read target id

				case $target in
				frame)
					;;

				client)
					transset -i $id --dec > /dev/null
					;;
				esac
			}
			;;

		*)
			echo unhandled event: $event $args >&2
			;;
		esac
	done
}

