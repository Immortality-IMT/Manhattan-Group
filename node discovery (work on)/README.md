The code is broken up into 3 parts

Part 1: Node discovery

Part 2: Propagation, latency

Part 3: Blockchain and Cryptocurrency

This directory is node discovery code. Part 1 and Part 2 are the peer to peer networks. Part 1 is how the peer to peer network discovers new nodes.

The main tenant of a peer to peer network is that all nodes are equal in function.

A node downloads and installs this code and with it a nodes.db

nodes.db is a sqlite3 database.

All nodes are sending their IP address and port to the GitHub nodes.db, so that new nodes can bootstrap themselves to the network.

Along with this, nodes are contacting other nodes and exchanging and updating their nodes.db

Any updates and improvement to the node discovery system should go here.

The code makes two threads, a server thread and a client thread, in the one program. The server thread accepts connection from nodes working to discover other nodes on the network. The client thread connects to the server from IP's and ports found in its nodes.db where they exchange nodes.db databases and update them.

A client connect will send its nodes.db and receive the server's nodes.db. Database functions then update the database with any new nodes found.

Node discovery is not mission-critical and therefore, nodes can search for other nodes, connecting to servers once per minute or even once per day.

The code is strictly C code.


Test
====

Simple compile and run the node_discovery

gcc -O0 -g -pthread node_discovery.c functions.h -o node_discovery -lsqlite

node_discovery can self-connect without crashing.

Additionally, change the port in the code to 5000 and 5001, generate two versions.

gcc -O0 -g -pthread node_discovery.c functions.h -o node_discovery5000 -lsqlite
gcc -O0 -g -pthread node_discovery.c functions.h -o node_discovery5001 -lsqlite

Each version will connect to eachother and exchange nodes.db
