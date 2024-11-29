
CFLAGS= -std=c99 -ggdb
LDFLAGS= -I.

all: build
	cc ${CFLAGS} -o build/game_of_life examples/game_of_life.c ${LDFLAGS}
	cc ${CFLAGS} -o build/lexer examples/lexer.c ${LDFLAGS}
	cc ${CFLAGS} -o build/simple_sm examples/simple_sm.c ${LDFLAGS}

build:
	mkdir build

