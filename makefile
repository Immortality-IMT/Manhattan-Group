# Master Makefile, makes all the code
# make wallet node_discovery propagation vm

# Blockchain
# gcc -g -o wallet transactions.c wallet.c verifications.c functions.h miner.c blockchain.c base58.c -lssl -lcrypto -lsqlite3 -lz -loqs

CC=gcc
CFLAGS=-g -lssl -lcrypto -lsqlite3 -lz -loqs -lm -pthread

wallet: blockchain_cryptocurrency/transactions.c blockchain_cryptocurrency/wallet.c blockchain_cryptocurrency/verifications.c blockchain_cryptocurrency/functions.h blockchain_cryptocurrency/miner.c blockchain_cryptocurrency/blockchain.c blockchain_cryptocurrency/base58.c
	$(CC) -o blockchain_cryptocurrency/wallet blockchain_cryptocurrency/transactions.c blockchain_cryptocurrency/wallet.c blockchain_cryptocurrency/verifications.c blockchain_cryptocurrency/miner.c blockchain_cryptocurrency/blockchain.c blockchain_cryptocurrency/base58.c $(CFLAGS)

# Node Discovery
# -pthread

node_discover: node_discovery/node_discovery.c node_discovery/functions.h
	$(CC) -o node_discovery/node_discover node_discovery/node_discovery.c $(CFLAGS)

# Propagation
# -pthread

propagation: propagation_latency/propagation.c propagation_latency/functions.h
	$(CC) -o propagation_latency/propagation propagation_latency/propagation.c $(CFLAGS)


# Smart Contracts
# -lm
vm: smart_contract_platform/vm.c
	$(CC) -o smart_contract_platform/vm smart_contract_platform/vm.c $(CFLAGS)
