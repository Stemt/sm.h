
CFLAGS= -std=c99 -ggdb

all: build
	cc -o build/gol examples/game_of_life.c
	cc -o build/lexer examples/lexer.c
	cc -o build/simple_sm examples/simple_sm.c

build:
	mkdir build

