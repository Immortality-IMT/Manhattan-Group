CC=gcc
CFLAGS=-g -pthread -lssl -lcrypto -lsqlite3 -lz -loqs

propagation: propagation.c functions.h
	$(CC) -o propagation propagation.c $(CFLAGS)
