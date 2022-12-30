#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <sqlite3.h>

typedef struct {
  char ip[16]; // IPv4 address
  int port;
  int sockfd; // socket descriptor
  int status; // 0 = disconnected, 1 = connected
} Node;

// Retrieves the number of records in the "nodes" table
int get_node_count(int *count) {
  sqlite3 *db;
  sqlite3_stmt *stmt;
  int rc;

  // Open the database connection
  rc = sqlite3_open("nodes.db", &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return 1;
  }

  // Prepare the SQL statement
  const char *sql = "SELECT COUNT(*) FROM nodes";
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return 1;
  }

  // Execute the statement
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return 1;
  }

  // Retrieve the count value
  *count = sqlite3_column_int(stmt, 0);

  // Finalize the statement and close the database connection
  sqlite3_finalize(stmt);
  sqlite3_close(db);

  return 0;
}

// Retrieves all records from the "nodes" table and stores them in the provided array
int get_nodes(Node *nodes) {
  sqlite3 *db;
  sqlite3_stmt *stmt;
  int rc;
  int i = 0;

  // Open the database connection
  rc = sqlite3_open("nodes.db", &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return 1;
  }

  // Prepare the SQL statement
  const char *sql = "SELECT ip, port FROM nodes";
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return 1;
  }

  // Iterate over the rows of the result set
  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    // Retrieve the values for the ip and port columns
    const unsigned char *ip = sqlite3_column_text(stmt, 0);
    int port = sqlite3_column_int(stmt, 1);

    // Store the values in the provided array
    strcpy(nodes[i].ip, (char*)ip);
    nodes[i].port = port;
    i++;
  }
  if (rc != SQLITE_DONE) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return 1;
  }

  // Finalize the statement and close the database connection
  sqlite3_finalize(stmt);
  sqlite3_close(db);

  return 0;
}


// Connects to a server at the given IP address and port
int connect_to_server(Node *node) {
  struct sockaddr_in server_addr;

  // Create the socket
  node->sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (node->sockfd < 0) {
    perror("Error creating socket");
    return 1;
  }

  // Set the socket to non-blocking mode
  int flags = fcntl(node->sockfd, F_GETFL, 0);
  if (flags < 0) {
    perror("Error getting socket flags");
    return 1;
  }
  flags |= O_NONBLOCK;
  if (fcntl(node->sockfd, F_SETFL, flags) < 0) {
    perror("Error setting socket to non-blocking mode");
    return 1;
  }

  // Set up the server address structure
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(node->port);
  if (inet_pton(AF_INET, node->ip, &server_addr.sin_addr) <= 0) {
    perror("Error parsing IP address");
    return 1;
  }

  // Connect to the server
  int rc = connect(node->sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (rc < 0) {
    if (errno != EINPROGRESS) {
      perror("Error connecting to server");
      return 1;
    }
  }

  node->status = 1;
  return 0;
}

// Sends a message to a server
int send_message(Node *node, char *message) {
  int message_length = strlen(message);

  // Send the message
  int bytes_sent = send(node->sockfd, message, message_length, 0);
  if (bytes_sent < 0) {
    perror("Error sending message");
    return 1;
  }

  return 0;
}

// Receives a message from a server
int receive_message(Node *node, char *message, int message_length) {
  int bytes_received = 0;
  int total_bytes_received = 0;

  // Use select to determine when the socket is readable
  fd_set read_fds;
  FD_ZERO(&read_fds);
  FD_SET(node->sockfd, &read_fds);
  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  int rc = select(node->sockfd + 1, &read_fds, NULL, NULL, &timeout);
  if (rc < 0) {
    perror("Error in select");
    return 1;
  }
  if (rc == 0) {
    // Timeout occurred
    return 2;
  }

  // Receive the message
  while (total_bytes_received < message_length) {
    bytes_received = recv(node->sockfd, message + total_bytes_received, message_length - total_bytes_received, 0);
    if (bytes_received < 0) {
      perror("Error receiving message");
      return 1;
    }
    if (bytes_received == 0) {
      // Connection closed
      node->status = 0;
      return 3;
    }
    total_bytes_received += bytes_received;
  }

  return 0;
}
#endif // FUNCTIONS_H_INCLUDED

