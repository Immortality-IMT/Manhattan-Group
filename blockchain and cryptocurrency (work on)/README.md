The code is broken up into 3 parts

Part 1: Node discovery

Part 2: Propagation, latency

Part 3: Blockchain and Cryptocurrency

This directory is blockchain and cryptocurrency code. Part 1 and Part 2 are the peer to peer networks. Part 3 is all the blockchain and cryptocurrency code.
Such as local transactions and hashing blocks, wallet, mining...

Once this data is packaged it is send to Part 2 to be distributed to the network.

Any updates and improvement to the node discovery system should go here.

The code here has no networking code.

Cryptocurrency and blockchain is mission critical and  relys on computing power to generate valid hases.

The code is strictly C code.

Test
====

Change the port in the code to 5000 and 5001, generate two versions.

gcc node_discovery.c -o usersport5000
gcc node_discovery.c -o usersport5001

Each version will connect to eachother and exchange nodes.db
