.include "../share/mk/top.mk"

SRC += src/args.c
SRC += src/chain.c
SRC += src/client.c
SRC += src/cmd.c
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

LFLAGS_hfwm += -lX11

.for src in ${SRC:Msrc/*.c}
${BUILD}/bin/hfwm: ${BUILD}/${src:R}.o
.endfor

