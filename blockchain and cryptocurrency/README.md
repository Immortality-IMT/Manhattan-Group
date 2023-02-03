# Blockchain

gcc -g -o wallet transactions.c wallet.c verifications.c functions.h miner.c blockchain.c base58.c keccak.c -lssl -lcrypto -lsqlite3 -lz

The code is broken up into 4 parts

Part 1: Node discovery

Part 2: Propagation, latency

Part 3: Blockchain and Cryptocurrency

Part 4: Smart contract platform

This directory is blockchain and cryptocurrency code. Part 1 and Part 2 are the peer to peer networks. Part 3 is all the blockchain and cryptocurrency code. Such as local transactions and hashing blocks, wallet, mining...

A blockchain consists of...

1) Wallet -> Generate keys, public, private & address. wallet.db
2) Transactions -> sender key, receiver key, amount, add to transactions.db (verifications, signatures)
3) Mine -> transactions into blocks...
4) Add blocks to blockchain, add to blockchain.db
   
5) Verifications, sha hashes, proofs, preferences, signatures...

Once this data is packaged, it is sent to Part 2 to be distributed to the network. The code here has no networking code.

Any updates and improvement to the node blockchain, cryptocurrency system should go here.

Cryptocurrency and blockchain is mission-critical and relies on computing power to generate valid hashes.

The code is strictly C code.
