#include "functions.h"


int main(int argc, char **argv) {
  int node_count;
  int MAX_MESSAGE_LENGTH = 1024;
  char message[MAX_MESSAGE_LENGTH];

  // Retrieve the number of records in the "nodes" table
  if (get_node_count(&node_count)) {
    return 1;
  }

  // Allocate space for the array of nodes
  Node *nodes = malloc(node_count * sizeof(Node));
  if (!nodes) {
    fprintf(stderr, "Error allocating memory for nodes\n");
    return 1;
  }


  // Retrieve all records from the database
  if (get_nodes(nodes)) {
    free(nodes);
    return 1;
  }

  // Echo retrieved records, debug
  for (int i = 0; i < node_count; i++) {
    printf("IP: %s, Port: %d\n", nodes[i].ip, nodes[i].port);
  }


  // Connect to each server and send a message
  for (int i = 0; i < node_count; i++) {
    if (connect_to_server(&nodes[i])) {
      continue;
    }
    if (send_message(&nodes[i], "AM/PM Wake up!")) {
      continue;
    }
      //sched_yield(); // Relinquish the CPU
  }

  // Receive messages from each server
  for (int i = 0; i < node_count; i++) {
    if (!nodes[i].status) {
      continue;
    }
    int rc = receive_message(&nodes[i], message, MAX_MESSAGE_LENGTH);
    if (rc == 0) {
      printf("Received message from server: %s\n", message);
    } else if (rc == 2) {
      printf("Timeout occurred while waiting for message from server\n");
    } else if (rc == 3) {
      printf("Connection closed by server\n");
    } else {
      printf("Error receiving message from server\n");
    }
  }

  // Free the allocated memory
  free(nodes);

  return 0;
}
