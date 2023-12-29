CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -g 
SRC_DIR = ./src/
all: lizard-client roaches-client wasp-client server display-app
	mkdir bin
	mkdir bin/server
	mkdir bin/clients
	mkdir bin/remote-display
	mkdir bin/clients/lizard
	mkdir bin/clients/cockroach
	mkdir bin/clients/wasp
	mv lizard-client bin/clients/lizard/
	mv roaches-client bin/clients/cockroach/
	mv wasp-client bin/clients/wasp/
	mv display-app bin/remote-display/
	mv server bin/server/
lizard-client: $(SRC_DIR)lizard-client.c $(SRC_DIR)remote-char.h
	$(CC) $(CFLAGS) -o lizard-client $(SRC_DIR)lizard-client.c $(SRC_DIR)auxfuncs.c -lncurses -lzmq -lm

roaches-client: $(SRC_DIR)roaches-client.c $(SRC_DIR)remote-char.h
	$(CC) $(CFLAGS) -o roaches-client $(SRC_DIR)roaches-client.c $(SRC_DIR)auxfuncs.c -lncurses -lzmq -lm

wasp-client: $(SRC_DIR)wasp-client.c $(SRC_DIR)remote-char.h
	$(CC) $(CFLAGS) -o wasp-client $(SRC_DIR)wasp-client.c $(SRC_DIR)auxfuncs.c -lncurses -lzmq -lm

server: $(SRC_DIR)server.c $(SRC_DIR)remote-char.h
	$(CC) $(CFLAGS) -o server $(SRC_DIR)server.c $(SRC_DIR)auxfuncs.c -lncurses -lzmq -lm

display-app: $(SRC_DIR)display-app.c $(SRC_DIR)remote-char.h
	$(CC) $(CFLAGS) -o display-app $(SRC_DIR)display-app.c $(SRC_DIR)auxfuncs.c -lncurses -lzmq -lm

clean:
	rm -rf bin
