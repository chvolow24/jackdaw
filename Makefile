CC := gcc
CFLAGS := -Wall -I/opt/homebrew/include/ `sdl2-config --libs --cflags` -lSDL2 -lSDL2_ttf
SRCS := audio.c text.c project.c theme.c gui.c wav.c main.c 
OBJS := $(SRCS:.c=.o)
EXEC := jackdaw

$(EXEC): $(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS)

clean:
	rm -f $(EXEC) $(OBJS)