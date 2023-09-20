CC := gcc
CFLAGS := -Wall -I/opt/homebrew/include/SDL2 `sdl2-config --libs --cflags` -lSDL2 -lSDL2_ttf
SRCS := audio.c text.c project.c theme.c gui.c wav.c draw.c timeline.c main.c
HDRS := audio.h text.h project.h theme.h gui.h wav.h
OBJS := $(SRCS:.c=.o)

EXEC := jackdaw

$(EXEC): $(OBJS) $(HDRS)
	$(CC) -o $@ $(OBJS) $(CFLAGS)

clean:
	rm -f $(EXEC) $(OBJS)