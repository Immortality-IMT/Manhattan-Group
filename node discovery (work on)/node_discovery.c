/*
----------------------------------------
P2P Implementation In C - Node Discovery
----------------------------------------
The strictly C program, has a server and a client in the one program.
They are a peer to peer program that performs the role of node discovery.
The server is one thread, the client is second thread.
They both update the same nodes file (nodes.db) upon interaction with.servers
(when acting as a client) and with clients (when acting as a server).
They exchange a nodes database.
Server updates its node file (nodes.db) when they are connected by clients
and receive the clients nodes.db.
Clients also update the same node file when they connect to servers
and receives the servers nodes.db
As the server is also a client (in the one program) it is connecting to servers as well as
receiving connects and both result in an update in the single node file (nodes.db).
The nodes.db file holds all the ips and port of the known network, that the node knows about.
Node discovery is about connecting to servers and grabbing their list of nodes database and vice versa.
An example P2P chat program would then parse the nodes.db file and connect to every ip to send a message.
An example P2P file sharing program would do the same, more extensively it would expand nodes.db to
include the files held by a particular node.
A cryptocurrency would send transactions and blocks and so on...
This program does not do anything other than seeking out nodes to update its nodes.db.
To program is able to self-connect successfully and you can also test by change the port and
generate two versions, usesport5000 and usesport5001, the binaries connect to eachother
(if their ips and ports are in their nodes.db) and share node files.
The nodes.db is an sqlite database.

There are two special cases, one is there is no easy way of getting the public ip address from within the network
or knowing when it changes. Second issue is posting the ip to main node.db, the node.db that is downloaded with
the main program, so new users get bootstraped on installation.

Commands:
gcc -O0 -g -pthread node_discovery.c functions.h -o node_discovery -lsqlite
gdb node_discovery core
bt
ps aux | grep node_discovery | awk '{print $2}' | xargs kill -9
*/


#include "functions.h"

char *my_ip; //Hold the local public IP globally
#define PORT 5001

// Global flag to track if the signal has been received
int sigintReceived = 0;

// Signal handler for SIGINT
void sigint_handler(int sig) {
    sigintReceived = 1;
}

// Thread function for the server thread
void *thread_server(void *arg) {

    // Create a socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
    perror("socket");
    //return 1;
    }

    // Set up the server address
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the server address
    if (bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
    perror("bind");
   // return 1;
    }

    // Listen for incoming connections
    listen(sockfd, 5);


    //loop the server
    while (!sigintReceived) {

            int ret = the_server(sockfd);
            // ret 1...

    }

    close(sockfd);
    return NULL;
}

// Thread function for the client thread
void *thread_client(void *arg) {

    //Get random record from nodes.db to connect to...
    int id;
    char *ip;
    int port;
    double uptime;
    int conecterror;
    int exp1;
    int exp2;
    int exp3;
    //Get random...

    while (!sigintReceived) {

        //Get a random record from nodes.db to connect to...
        int ret = get_random_node(&id, &ip, &port, &uptime, &conecterror, &exp1, &exp2, &exp3);
        if( ret != 0 ) {
            fprintf(stderr, "Error getting random node\n");
        }

        /* The values of the returned record */
        printf("ID: %d\n", id);
        printf("IP: %s\n", ip);
        printf("Port: %d\n", port);
        printf("Uptime: %f\n", uptime);
        printf("Attempts: %d\n", conecterror);
        printf("Exp1: %d\n", exp1);
        printf("Exp2: %d\n", exp2);
        printf("Exp3: %d\n", exp3);


        //Connect to the random server
        ret = connect_to_server(ip, port);


        //Process the exchanged db
        if (ret == 0) {

                ret = process_nodes_db();

        }

        sleep(5);

    }

    return NULL;
}

// Thread function for general tasks
void *thread_tasks(void *arg) {

/* tasks:
    1: Get public ip every 12 hours
    2: Add that ip to the github repos nodes.db
    3: Add that ip to the local nodes.db
*/

    while (!sigintReceived) {

        char *response = get_public_ip();
            if (response == NULL) {
                fprintf(stderr, "Failed to get public IP\n");
                //return 1;
            }

        //char *ip = extract_ipv4(response);
        my_ip = extract_ipv4(response);
            if (my_ip == NULL) {
                fprintf(stderr, "Failed to extract IPv4 address\n");
                //return 1;
            }

        printf("Public IP is: %s\n", my_ip);
        //my_ip is global variable
        free(response);

        //Put it into the local nodes.db
        insert_node(my_ip, PORT); //publicy ip, PORT for other nodes to know

        //Post it the main repo nodes.db
        //TODO

        // Sleep for 12 hours
        sleep(12 * 60 * 60);
    }

    return NULL;
}

int main(void) {
    // Set up the signal handler for SIGINT
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = sigint_handler;
    sigaction(SIGINT, &action, NULL);

    // Create the threads
    pthread_t thread_server_id, thread_client_id, thread_tasks_id;
    int ret = pthread_create(&thread_server_id, NULL, thread_server, NULL);
    if (ret != 0) {
        perror("Error creating thread server");
        exit(1);
    }
    ret = pthread_create(&thread_client_id, NULL, thread_client, NULL);
    if (ret != 0) {
        perror("Error creating thread client");
        exit(1);
    }
    ret = pthread_create(&thread_tasks_id, NULL, thread_tasks, NULL);
    if (ret != 0) {
        perror("Error creating thread tasks");
        exit(1);
    }

    // Wait for the threads to finish
    pthread_join(thread_server_id, NULL);
    pthread_join(thread_client_id, NULL);
    pthread_join(thread_tasks_id, NULL);

    return 0;
}
