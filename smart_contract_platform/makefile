CC=gcc
CFLAGS=-g -pthread -lssl -lcrypto -lsqlite3 -lz -loqs -lm

vm: vm.c
	$(CC) -o vm vm.c $(CFLAGS)
