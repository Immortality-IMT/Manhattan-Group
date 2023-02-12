#gcc -g -o wallet transactions.c wallet.c verifications.c functions.h miner.c blockchain.c base58.c -lssl -lcrypto -lsqlite3 -lz -loqs

CC=gcc
CFLAGS=-g -lssl -lcrypto -lsqlite3 -lz -loqs

wallet: transactions.c wallet.c verifications.c functions.h miner.c blockchain.c base58.c
	$(CC) -o wallet transactions.c wallet.c verifications.c miner.c blockchain.c base58.c $(CFLAGS)

