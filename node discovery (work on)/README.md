The code is broken up into 3 parts

Part 1: Node discovery

Part 2: Propagation, latency

Part 3: Blockchain and Cryptocurrency

This directory is node discovery code. Part 1 and Part 2 are the peer to peer networks. Part 1 is how the peer to peer network discovers new nodes.

A node downloads and install the code and with it a node.db.

All nodes are sending their IP address and port to the this node.db, so that a new node can bootstrap itself to the network.

Along with this nodes are contacting other nodes and exchanging and updating their node.db.

Any updates and improvement to the node discovery system should go here.

The code here is a server and a client in the one program. The server is the main program and accept connection from nodes working to discover other nodes
on the network. The client is a thread that connects to server from its node.db where they exchange node.db database and updat them.

Node discovery is not mission critical and therefore, nodes could only search for other nodes connecting to another server once per minute or even once per day.

The code is strictly C code.
