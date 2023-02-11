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

Why Use Proof of Work (POW)
---------------------------
For miners, and for crypto miners to advance the investment into renewable energy projects and investment into faster computer hardware.

Why Use a Relational Database? (SQL)
------------------------------------
Relational database is a proven technology with many tools. A web application can run SQL statements against the blockchain, a shopping cart can return if a transaction has succeeded on not. Another application that run specific search queries.

Wny Use Next Gen Cryptography
-----------------------------
Cryptography has imporoved since Bitcoin's first version. It becomes a  major effort to move Bitcoin and most other cryptocurrencues to a more modern encryption. Modern cryptocurrency blockchain implementations should be using Sha3-384 and Dilithium2.

The implementation of Dilithium2 is using OQS lib.

APT...

 sudo apt install astyle cmake gcc ninja-build libssl-dev python3-pytest python3-pytest-xdist 
 git clone -b main https://github.com/open-quantum-safe/liboqs.git
 cd liboqs

build:

 mkdir build && cd build
 cmake -GNinja -DBUILD_SHARED_LIBS=ON ..
 ninja
 ninja install

if /usr/local/lib is not in $PATH
 
 export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib

What is your TPS (transactions per second)
------------------------------------------
We have not benchmarked TPS as yet and we are aiming for Mastercard, Visa level TPS, which is about 2,000 TPS which can be achieved with sharding-based Proof-of-Work (PoW) such as Zilliqa, boasting 2,828 transactions per second and uses an unusual consensus mechanism that combines proof-of-work (PoW) and Practical Byzantine Fault Tolerance (pBFT).
