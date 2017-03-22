.include "../share/mk/top.mk"

SRC += src/args.c
SRC += src/chain.c
SRC += src/client.c
SRC += src/cmd.c
SRC += src/current.c
SRC += src/event.c
SRC += src/frame.c
SRC += src/geom.c
SRC += src/key.c
SRC += src/layout.c
SRC += src/main.c
SRC += src/order.c
SRC += src/spawn.c
SRC += src/tile.c
SRC += src/win.c

PROG += hfwm

CFLAGS += ${CFLAGS.x11}
LFLAGS.hfwm += ${LIBS.x11}

.for src in ${SRC:Msrc/*.c}
${BUILD}/bin/hfwm: ${BUILD}/${src:R}.o
.endfor

