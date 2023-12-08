CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -g 

all: human-client machine-client server-exercise-3 remote-display-client

human-client: human-client.c remote-char.h
	$(CC) $(CFLAGS) -o human-client human-client.c auxfuncs.c -lncurses -lzmq -lm

machine-client: machine-client.c remote-char.h
	$(CC) $(CFLAGS) -o machine-client machine-client.c auxfuncs.c -lncurses -lzmq -lm

server-exercise-3: server-exercise-3.c remote-char.h
	$(CC) $(CFLAGS) -o server-exercise-3 server-exercise-3.c auxfuncs.c -lncurses -lzmq -lm

remote-display-client: remote-display-client.c remote-char.h
	$(CC) $(CFLAGS) -o remote-display-client remote-display-client.c auxfuncs.c -lncurses -lzmq -lm

clean:
	rm -f human-client machine-client server-exercise-3 remote-display-client
