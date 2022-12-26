The code is broken up into 3 parts

Part 1: Node discovery

Part 2: Propagation, latency

Part 3: Blockchain and Cryptocurrency

This directory is the propagation and latency code. Part 1 and Part 2 are the peer to peer networks. Part 2 is how transactions and block propagate and are latent
across the network.

A node has a trnasaction or a block to propagate to the peer to peer network. To be included a mempool or blockpool.

This code takes that data and sends to every node in the network as fast and as efficient as possible. Form propagation of new transactions, new blocks and syncing the blockchain.

The aim is that propagation be without latentcy and fast.

Any updates and improvement to the propagation and latency should go here.

While the node discovery is maintaining the network recognition, propagation and latency is network residence.

The code here is also a server and a client in the one program. The server is the main program and accept connection from nodes with transactions, blocks and data. That new data activates the node to also send out that data to other nodes. 
The client is a thread that connects to server from its node.db where they exchange transaction and blockchain data.

Their is a rush condition and that is repeat connections of data a node already has.

Propagation and latency is mission critical and therefore, nodes need to do as much as possible in the shortest period of time to propagate a transaction
throughout the entire network.

The code is strictly C code.
