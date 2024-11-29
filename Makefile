
CFLAGS= -std=c99 -ggdb -Wall -Wextra
LDFLAGS= -I.

all: build
	cc ${CFLAGS} -o build/game_of_life examples/game_of_life.c ${LDFLAGS}
	cc ${CFLAGS} -o build/lexer examples/lexer.c ${LDFLAGS}
	cc ${CFLAGS} -o build/simple_sm examples/simple_sm.c ${LDFLAGS}

test: build
	# utest.h requires > c99 for nice printing (https://github.com/sheredom/utest.h/issues/81)
	cc -ggdb -o build/test tests/test.c ${LDFLAGS}
	./build/test

build:
	mkdir build

