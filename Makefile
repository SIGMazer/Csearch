CC=cc
CLIBS= -lm
CFLAGS= -Wall -Wextra -ggdb -pedantic

main: main.c 
	$(CC) $(CFLAGS) -o main main.c $(CLIBS)
