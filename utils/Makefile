# Makefile for RBDoom3-BFG utilities
#

all: test_gamepad_SDL test_gamepad_SDL2

test_gamepad_SDL: test_gamepad_SDL.cpp
	gcc -g -o test_gamepad_SDL test_gamepad_SDL.cpp -lSDL

test_gamepad_SDL2: test_gamepad_SDL2.cpp
	gcc -g -o test_gamepad_SDL2 test_gamepad_SDL2.cpp -lSDL2

clean:
	rm -f test_gamepad_SDL
	rm -f test_gamepad_SDL2
