CC=gcc
CFLAGS=-g -pthread -lssl -lcrypto -lsqlite3 -lz -loqs

node_discovery: node_discovery.c functions.h
	$(CC) -o node_discover node_discovery.c $(CFLAGS)
