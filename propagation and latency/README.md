The code is broken up into 4 parts

Part 1: Node discovery

Part 2: Propagation, latency

Part 3: Blockchain and Cryptocurrency

Part 4: Smart contract platform

This directory is the propagation and latency code. Part 1 and Part 2 are the peer to peer networks. Part 2 is how transactions and block propagate and are latent across the network.

A node has a transaction or a block to propagate to the peer to peer network. To be included in a mempool or blockpool.

This code takes that data and sends it to every node in the network as fast and as efficient as possible. From propagation of new transactions, new blocks and syncing the blockchain.

The aim is that propagation be without latency and fast. The winning strategy is speed, how fast can a transaction, block, blockchain update reach the entire network.

Any updates and improvement to the propagation and latency should go here.

While the node discovery is maintaining the network recognition, propagation and latency is network residence.

The code here is also a server and a client in the one program. The server is the main program and accept connection from nodes with transactions, blocks and data. That new data activates the node to also send out that data to other nodes. The client is a thread that connects to the server from its node.db where they exchange transaction and blockchain data.

At this stage there is no rush condition, a rush condition could occur if nodes pushed transaction data on behalf of other nodes which already have the transaction data, minimizing repeat connections.

Propagation and latency is mission-critical and therefore, nodes need to do as much as possible in the shortest period of time to propagate a transaction throughout the entire network.

The code is strictly C code.
