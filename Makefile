CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -g 
SRC_DIR = ./src/
all: lizard-client roaches-client wasp-client server
	mkdir bin
	mkdir bin/server
	mkdir bin/clients
	mkdir bin/clients/lizard
	mkdir bin/clients/cockroach
	mkdir bin/clients/wasp
	mkdir bin/clients/cockroach/python
	mv lizard-client bin/clients/lizard/
	mv roaches-client bin/clients/cockroach/
	mv wasp-client bin/clients/wasp/
	mv server bin/server/
	cp ./src/message_pb2* bin/clients/cockroach/python/
	cp ./src/roaches-client.py bin/clients/cockroach/python/

lizard-client: $(SRC_DIR)lizard-client.c $(SRC_DIR)remote-char.h
	$(CC) $(CFLAGS) -o lizard-client $(SRC_DIR)lizard-client.c $(SRC_DIR)auxfuncs.c  $(SRC_DIR)message.pb-c.c -lncurses -lzmq -lm -lprotobuf-c -lpthread

roaches-client: $(SRC_DIR)roaches-client.c $(SRC_DIR)remote-char.h
	$(CC) $(CFLAGS) -o roaches-client $(SRC_DIR)roaches-client.c $(SRC_DIR)auxfuncs.c $(SRC_DIR)message.pb-c.c -lncurses -lzmq -lm -lprotobuf-c

wasp-client: $(SRC_DIR)wasp-client.c $(SRC_DIR)remote-char.h
	$(CC) $(CFLAGS) -o wasp-client $(SRC_DIR)wasp-client.c $(SRC_DIR)auxfuncs.c $(SRC_DIR)message.pb-c.c -lncurses -lzmq -lm -lprotobuf-c

server: $(SRC_DIR)server.c $(SRC_DIR)remote-char.h
	$(CC) $(CFLAGS) -o server $(SRC_DIR)server.c $(SRC_DIR)auxfuncs.c -lncurses $(SRC_DIR)message.pb-c.c -lzmq -lm -lprotobuf-c

clean:
	rm -rf bin
