CC=cc
CLIBS= -lm
CFLAGS= -Wall -Wextra -ggdb -pedantic  -O3

csearch: main.c 
	$(CC) $(CFLAGS) -o csearch main.c $(CLIBS)
