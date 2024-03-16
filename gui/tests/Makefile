# Directories
SRC_DIR := ../src
INC_DIR := ../include
BUILD_DIR := ../build

CFLAGS := `sdl2-config --cflags --libs` -lSDL2_ttf  -I../include

test.o : ../build/*.o
	gcc test.c -o test.o $(CFLAGS)
