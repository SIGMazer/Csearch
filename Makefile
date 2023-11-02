CC=cc
CLIBS= -lm
CFLAGS= -Wall -Wextra -ggdb -pedantic  -O3

main: main.c 
	$(CC) $(CFLAGS) -o main main.c $(CLIBS)
