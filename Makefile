CC := gcc
CFLAGS := -Wall -I/opt/homebrew/include/ `sdl2-config --libs --cflags` -lSDL2 -lSDL2_ttf
HDRS := 
SRCS := main.c audio.c text.c
OBJS := $(SRCS:.c=.o)
EXEC := jackdaw

$(EXEC): $(OBJS) $(HDRS)
	$(CC) -o $@ $(OBJS) $(CFLAGS)

clean:
	rm -f $(EXEC) $(OBJS)