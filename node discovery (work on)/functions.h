#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>
#include <regex.h>
#include <sqlite3.h>

/**SQLite3**/
int get_random_node(int *id, char **ip, int *port, double *uptime, int *conecterror, int *exp1, int *exp2, int *exp3);
int connect_to_server(char* ip, int port);
int process_nodes_db();
int the_server(int sockfd);

//Return a random Sqlite record
int get_random_node(int *id, char **ip, int *port, double *uptime, int *conecterror, int *exp1, int *exp2, int *exp3) {
    sqlite3 *db;
    int rc;

    /* Allocate memory for the ip string */
    *ip = (char *)malloc(16);
    if( *ip == NULL ) {
        fprintf(stderr, "Error allocating memory for ip\n");
        return 1;
    }

    /* Open database */
    rc = sqlite3_open("nodes.db", &db);
    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    /* Select a random record from the table */
    char *sql = "SELECT * FROM nodes ORDER BY RANDOM() LIMIT 1;";

    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if( rc != SQLITE_OK ) {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    /* Execute the statement and process the result */
    rc = sqlite3_step(stmt);
    if( rc == SQLITE_ROW ) {
        /* Return values as function arguments */
        *id = sqlite3_column_int(stmt, 0);
        strcpy(*ip, (char*)sqlite3_column_text(stmt, 1));
        //*ip = (char*)sqlite3_column_text(stmt, 1);
        *port = sqlite3_column_int(stmt, 2);
        *uptime = sqlite3_column_double(stmt, 3);
        *conecterror = sqlite3_column_int(stmt, 4);
        *exp1 = sqlite3_column_int(stmt, 5);
        *exp2 = sqlite3_column_int(stmt, 6);
        *exp3 = sqlite3_column_int(stmt, 7);
    } else {
        fprintf(stderr, "No rows returned\n");
        return 1;
    }

    /* Close the statement and database connection */
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return 0;
}

int connect_to_server(char* ip, int port) {

  // Create a socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("socket");
    return 1;
  }

  // Set up the server address
  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) < 1) {
    perror("inet_pton");
    return 1;
  }

  // Connect to the server
  if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
    perror("connect");
    return 1;
  }

  // Open the file to be sent
  FILE *fp = fopen("nodes.db", "rb");
  if (fp == NULL) {
    perror("fopen");
    return 1;
  }

  // Get the file size
  fseek(fp, 0, SEEK_END);
  int file_size = ftell(fp);
  rewind(fp);

  // Send the file size to the server
  if (send(sockfd, &file_size, sizeof(file_size), 0) < 0) {
    perror("send");
    return 1;
  }

  // Send the file to the server
  char buffer[1024];
  int bytes_read;
  while ((bytes_read = fread(buffer, 1, 1024, fp)) > 0) {
    if (send(sockfd, buffer, bytes_read, 0) < 0) {
      perror("send");
      return 1;
    }
  }

  // Receive the file size from the server
  int recv_file_size;
  if (recv(sockfd, &recv_file_size, sizeof(recv_file_size), 0) < 0) {
    perror("recv");
    return 1;
  }

  // Allocate memory for the file
  char *recv_buffer = malloc(recv_file_size);
  if (recv_buffer == NULL) {
    perror("malloc");
    return 1;
  }

  // Receive the file from the server
  int bytes_received = 0;
  while(bytes_received < recv_file_size) {
    int result = recv(sockfd, recv_buffer + bytes_received, recv_file_size - bytes_received, 0);
    if (result < 0) {
    perror("recv");
    return 1;
    }
    bytes_received += result;
  }

    // Save the received file to a local file
    FILE *recv_fp = fopen("nodes_.db", "wb");
    if (recv_fp == NULL) {
    perror("fopen");
    return 1;
    }
    fwrite(recv_buffer, 1, recv_file_size, recv_fp);

    // Clean up
    fclose(fp);
    fclose(recv_fp);
    free(recv_buffer);
    close(sockfd);

  return 0;
}


int process_nodes_db() {
    sqlite3 *db1;
    sqlite3 *db2;
    int rc;

    // Open the first database
    rc = sqlite3_open("nodes.db", &db1);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error: %s\n", sqlite3_errmsg(db1));
        sqlite3_close(db1);
        return 1;
    }

    // Check if the second database file exists
    if (access("nodes_.db", F_OK) == -1) {
        // The file does not exist, exit the program
        fprintf(stderr, "Error: The second database file does not exist.\n");
        sqlite3_close(db1);
        return 1;
    }

    // Open the second database
    rc = sqlite3_open("nodes_.db", &db2);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error: %s\n", sqlite3_errmsg(db2));
        sqlite3_close(db2);
        return 1;
    }

    // Select data from the second database
    sqlite3_stmt *stmt;
    const char *select_sql = "SELECT * FROM nodes";
    rc = sqlite3_prepare(db2, select_sql, strlen(select_sql)+1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error: %s\n", sqlite3_errmsg(db2));
        sqlite3_close(db2);
        return 1;
    }

    // Insert data into the first database
  while(sqlite3_step(stmt) == SQLITE_ROW ){
        sqlite3_stmt *insert_stmt;
//        const char *insert_sql = "INSERT OR IGNORE INTO nodes (ip, port, uptime, conecterror, exp1, exp2, exp3) VALUES (?, ?, ?, ?, ?, ?, ?)";
        const char *insert_sql = "INSERT INTO nodes (ip, port, uptime, conecterror, exp1, exp2, exp3) VALUES (?, ?, ?, ?, ?, ?, ?) ON CONFLICT (ip, port) DO NOTHING";


        rc = sqlite3_prepare(db1, insert_sql, strlen(insert_sql)+1, &insert_stmt, 0);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Error: %s\n", sqlite3_errmsg(db1));
            sqlite3_close(db1);
            return 1;
        }

        sqlite3_bind_text(insert_stmt, 1, sqlite3_column_text(stmt, 1), -1, SQLITE_STATIC);
        sqlite3_bind_int(insert_stmt, 2, sqlite3_column_int(stmt, 2));
        sqlite3_bind_double(insert_stmt, 3, sqlite3_column_double(stmt, 3));
        sqlite3_bind_int(insert_stmt, 4, sqlite3_column_int(stmt, 4));
        sqlite3_bind_int(insert_stmt, 5, sqlite3_column_int(stmt, 5));
        sqlite3_bind_int(insert_stmt, 6, sqlite3_column_int(stmt, 6));
        sqlite3_bind_int(insert_stmt, 7, sqlite3_column_int(stmt, 7));

        if (sqlite3_step(insert_stmt) != SQLITE_DONE) {
            fprintf(stderr, "Error: %s\n", sqlite3_errmsg(db1));
        }

        sqlite3_reset(insert_stmt);
    }

    // Close the prepared statement and the second database
    sqlite3_finalize(stmt);
    sqlite3_close(db2);

    // Close the first database
    sqlite3_close(db1);

    //Delete the temp database
    if (unlink("nodes_.db") != 0) {
        fprintf(stderr, "Error: Could not delete s\n");
    }

    return 0;
}

int the_server(int sockfd) {

/*
    // Create a socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
    perror("socket");
    return 1;
    }

    // Set up the server address
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the server address
    if (bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
    perror("bind");
    return 1;
    }

    // Listen for incoming connections
    listen(sockfd, 5);

    -------------
  */
    // Accept an incoming connection
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_sockfd = accept(sockfd, (struct sockaddr*) &client_addr, &client_addr_len);
    if (client_sockfd < 0) {
    perror("accept");
    return 1;
    }

    // Receive the file size from the client
    int file_size;
    if (recv(client_sockfd, &file_size, sizeof(file_size), 0) < 0) {
    perror("recv");
    return 1;
    }

    // Allocate memory for the file
    char *buffer = malloc(file_size);
    if (buffer == NULL) {
    perror("malloc");
    return 1;
    }

    // Receive the file from the client
    int bytes_received = 0;
    while (bytes_received < file_size) {
    int result = recv(client_sockfd, buffer + bytes_received, file_size - bytes_received, 0);
    if (result < 0) {
    perror("recv");
    return 1;
    }
    bytes_received += result;
    }

    // Save the received file to a local file
    FILE *fp = fopen("nodes_.db", "wb");
    if (fp == NULL) {
    perror("fopen");
    return 1;
    }
    fwrite(buffer, 1, file_size, fp);

    // Open the file to be sent
    FILE *send_fp = fopen("nodes.db", "rb");
    if (send_fp == NULL) {
    perror("fopen");
    return 1;
    }

    // Get the file size
    fseek(send_fp, 0, SEEK_END);
    int send_file_size = ftell(send_fp);
    rewind(send_fp);

    // Send the file size to the client
    if (send(client_sockfd, &send_file_size, sizeof(send_file_size), 0) < 0) {
    perror("send");
    return 1;
    }

    // Send the file to the client
    char send_buffer[1024];
    int bytes_read;
    while ((bytes_read = fread(send_buffer, 1, 1024, send_fp)) > 0) {
    if (send(client_sockfd, send_buffer, bytes_read, 0) < 0) {
    perror("send");
    return 1;
    }
    }

    // Clean up
    fclose(fp);
    fclose(send_fp);
    free(buffer);
    close(client_sockfd);

    //----

    //close(sockfd);

  return 0;
}

char *get_public_ip() {

    int BUFFER_SIZE = (2 * 1024 * 1024);
    int MYIP_BUFFER = 1024;         // Get public IP
    char buffer[MYIP_BUFFER];

    const char *services[4] = {     // List third party ret public IP services
        "api.ipify.org",
        "checkip.dyndns.org",
        "v4.ident.me",
        "ident.me"
    };

    int rand_num = rand() % 3; // Select a random service
    const char *hostname = services[rand_num];

  // Resolve hostname to address
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  struct addrinfo *result;
  int error;
  if ((error = getaddrinfo(hostname, "http", &hints, &result)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
    return NULL;
  }

    fprintf(stderr, "Public IP Server: %s and randnum = %d\n", hostname, rand_num);

  // Create socket and connect to server
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("socket");
    return NULL;
  }

  if (connect(sockfd, result->ai_addr, result->ai_addrlen) < 0) {
    perror("connect");
    return NULL;
  }

  // Build and send HTTP request
  char request[MYIP_BUFFER];
  int request_length = snprintf(request, BUFFER_SIZE, "GET /?format=text HTTP/1.0\r\n\r\n");
  if (send(sockfd, request, request_length, 0) < 0) {
    perror("send");
    return NULL;
  }

  // Receive response
  char *response = malloc(MYIP_BUFFER);
  int response_length = 0;
  int bytes_received;
  while ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
    buffer[bytes_received] = '\0';
    response_length += bytes_received;
    response = realloc(response, response_length + 1);
    strcat(response, buffer);
  }
  if (bytes_received < 0) {
    perror("recv");
    return NULL;
  }

  // Close socket
  close(sockfd);

  return response;
}

char *extract_ipv4(char *str) {

  static const char* pattern = "(([0-9]{1,3}\\.){3}[0-9]{1,3})"; //IP regex
  // Check if regular expression is already compiled
  static regex_t regex;
  static int compiled = 0;

  if (!compiled) {
    // Compile regular expression
    int ret = regcomp(&regex, pattern, REG_EXTENDED);
    if (ret) {
      char msgbuf[100];
      regerror(ret, &regex, msgbuf, sizeof(msgbuf));
      fprintf(stderr, "Could not compile regex: %s\n", msgbuf);
      return NULL;
    }
    compiled = 1;
  }

  // Execute regular expression
  regmatch_t matches[2];
  int ret = regexec(&regex, str, 2, matches, 0);
  if (!ret) {
    // Allocate memory for the IPv4 address
    char *ip = malloc(16);
    memcpy(ip, &str[matches[1].rm_so], matches[1].rm_eo - matches[1].rm_so);
    ip[matches[1].rm_eo - matches[1].rm_so] = '\0';
    return ip;
  } else if (ret == REG_NOMATCH) {
    printf("No match\n");
    return NULL;
  } else {
    char msgbuf[100];
    regerror(ret, &regex, msgbuf, sizeof(msgbuf));
    fprintf(stderr, "Regex match failed: %s\n", msgbuf);
    return NULL;
  }
}

int insert_node(const char* ip, int port) {
    sqlite3* db;
    char* error_message = 0;
    int rc;

    rc = sqlite3_open("nodes.db", &db);
    if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return 1;
    }

    char sql[256];
    sprintf(sql, "INSERT OR IGNORE INTO nodes (ip, port, uptime, conecterror, exp1, exp2, exp3) VALUES ('%s', %d, 0, 0, 0, 0, 0)", ip, port);

    rc = sqlite3_exec(db, sql, 0, 0, &error_message);
    if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", error_message);
    sqlite3_free(error_message);
    sqlite3_close(db);
    return 1;
}

 sqlite3_close(db);
 return 0;
}

#endif // FUNCTIONS_H_INCLUDED
