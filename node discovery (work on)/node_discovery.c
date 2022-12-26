/*
----------------------------------------
P2P Implementation In C - Node Discovery
----------------------------------------

The strictly C program, has a server and a client in the one program.
They are a peer 2 peer program that performs the role of node discovery.
The server is the main program, the client is a thread.
They both update the same nodes file (nodes.db) upon interaction with...
servers (when acting as a client) and with clients (when acting as a server).

They exchange a nodes database.

Server updates its node file (nodes.db) when they are connected by clients.
Clients also update the same node file when they connect to servers.
As the server is also acting as a client (in the one program) it is connecting to servers as well as
receiving connects and both result in an update in the single node file (nodes.db).

The nodes.db file holds all the ips and port of the known network, that the node knows about.

Node discovery is about connecting to servers and grabbing their list of nodes database and vice versa.

An example P2P chat program would then parse the nodes.db file and connect to every ip to send a message.
An example P2P file sharing program would do the same if it wanted everyone to hold the file, more
extensively it would expand nodes.db to include the files held by a particular node.
A cryptocurrency would send blockchain updates and so on...

This program does not do anything other than seeking out nodes to update its nodes.db.

To test this, change the port and generate two versions, usesport5000 and usesport5001 are
binaries that connect to eachother and share their node files.
Sample node.db format is [ip, port, expansion data 1, expansion data 2 (no spaces, no newlines)
The left bracket is the record delimiter, the commas are the field delimiters.
[127.0.0.1,5000,34,43[127.0.0.1,5001,43,54
ips.txt is being used for testing purposes so the file is not overwritten
contains the details of the over test server, resulting in a self connection.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>
#include <netdb.h>
#include <regex.h>


#define MAX_RECORD_LENGTH 256
#define MAX_FIELD_LENGTH 16
#define FIELD_DELIMITER ","             //nodes.db
#define DELIMITER '['                   //nodes.db
#define PORT 5000                       // Port number to listen on
#define BACKLOG 10 // Maximum number of pending connections in the queue
#define BUFFER_SIZE (2 * 1024 * 1024)   // Buffer size for nodes.db

//Do not connect to self by finding public IP
#define REGEX_PATTERN "(([0-9]{1,3}\\.){3}[0-9]{1,3})"
//Service that offer what is my public IP service
const char *services[4] = {
    "api.ipify.org",
    "checkip.dyndns.org",
    "v4.ident.me",
    "ident.me"
};

char *my_ip;

#define REGEX_BUFFER_SIZE 100   // Extract IP from HTTP header
#define MYIP_BUFFER 1024       // Get public IP

//char buffer[MYIP_BUFFER];

// Constants for the delay time (in seconds), check if pub ip has changed
const int DELAY_TIME = 12 * 60 * 60;

// Variables for measuring elapsed time, check if pub ip has changed
clock_t start_time, current_time;

// Combine nodes db leaving out duplicates
// Define a structure to store the IP address and port number
typedef struct {
  char ip[16]; // IP address (e.g. "123.123.123.123")
  int port;    // Port number
  char ext1[3];
  char ext2[3];
} record_t;

// Flag to keep track of whether the thread should end
bool thread_should_end = false;

// Global flag variable to track whether the loop should be terminated
volatile sig_atomic_t sigintReceived = 0;

int main(void);
int srv_listen(int sockfd, int backlog); //server
int srv_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen); //server
int srv_send(int sockfd, const void *buf, size_t len, int flags); //server
int srv_recv(int sockfd, void *buf, size_t len, int flags); //server
int srv_close(int fd);
void error(const char *msg);
void bzero(void *buffer, size_t length);
int usleep(unsigned int useconds);
//void *thread_function(void *arg); //connect to servers, node discovery
void *thread_function();
void handle_signal(int sig); //CTRL+C to end the server
int get_random_record(char* filename, char* buffer); //node discovery get an ip and port to connect to
int split_record(char* record, char** fields);
char *read_file_to_string(const char *filename);
char *generate_word();
char *read_from_socket_and_save_to_file(int sockfd);
// Function prototype for combine_files()
int combine_files(char *file1, char *file2, char *output_file);
// Function to delete a temp file
int delete_temp_file(const char* file_name);
// Function to rename a file
int rename_file(const char* old_name, const char* new_name);

// This is the thread function that will be executed by the new thread
//void *thread_function(void *arg) {
void *thread_function() {

    sleep(10);

    //Client connect
    int Csockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer2[BUFFER_SIZE];

       char *fields[4];

  // Generate a random seed based on the current time
  srand(time(NULL));

  printf("Discover nodes thread activated!\n");
  while (!thread_should_end) {

    // Call the function to get a random IP and port from the text file "ips.txt"
    // Get a random record from the file "records.txt"
    char buffer[MAX_RECORD_LENGTH];
    //if (get_random_record("ips.txt", buffer) == 0) {
    if (get_random_record("nodes.db", buffer) == 0) {
    printf("Random record: %s\n", buffer);
    }

    if (strlen(buffer) > 9) {
    for (int i = 0; i < 4; i++) {
    fields[i] = malloc(MAX_FIELD_LENGTH);
    }
    int num_fields = split_record(buffer, fields);
    printf("Number of fields: %d\n", num_fields);
    for (int i = 0; i < num_fields; i++) {
    printf("Field %d: %s\n", i + 1, fields[i]);
    }
   }

   //Connect to a server

    //usage %s hostname port\n", argv[0]);

    Csockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (Csockfd < 0)
        fprintf(stderr,"ERROR, opening socket\n");
        //error("ERROR opening socket");
    server = gethostbyname(fields[0]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
       // exit(0);
    } else {
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;

        //memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
        bcopy((char *)server->h_addr,
            (char *)&serv_addr.sin_addr.s_addr,
            server->h_length);

           int port = atoi(fields[1]);

        serv_addr.sin_port = htons(port);

         // Set the timeout value for the connection attempt
        struct timeval timeout = {10, 0};  // 10 seconds
        if (setsockopt(Csockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        {
        perror("setsockopt");
        //return 1;
        }


        if (connect(Csockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
            fprintf(stderr,"ERROR, connecting\n");
            //error("ERROR connecting");
        } else {
        // Send a request to the server for the data
            bzero(buffer2, BUFFER_SIZE);

                char *contents = read_file_to_string("nodes.db");
                if (contents == NULL) {
                // Handle error
                }


                //char response[] = contents; //"Hello, world!";
                //int bytes_sent = srv_send(newsockfd, contents, strlen(contents), 0);


            strcpy(buffer2, contents); //send your nodes files
            n = write(Csockfd, buffer2, strlen(buffer2));
            printf("Send Message: %s\n", contents);

        if (n < 0) {
            fprintf(stderr,"ERROR, writing to socket\n");
            //error("ERROR writing to socket");
        } else {
            //Get its node list
            char *file2 = read_from_socket_and_save_to_file(Csockfd);
            //update existing node list by adding new ones
            combine_files("nodes.db", file2, "output_file.txt");

        }

        }


    }
    // Close the socket connection
        close(Csockfd);

    sleep(10);

    //free the fields
    //for (int i = 0; i < 4; i++) {
    //free(fields[i]);
    //}

  }
  printf("Thread ending\n");
  return NULL;
}

int srv_listen(int sockfd, int backlog) {

  // Set the socket to listen for incoming connections
  if (listen(sockfd, backlog) < 0) {
    error("ERROR: Failed to listen on socket");
    return -1;
  }

    // Set the socket to non-blocking mode
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

  return 0;
}

int srv_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
  // Accept an incoming connection
  int newsockfd = accept(sockfd, addr, addrlen);

  // Check for errors
  if (newsockfd < 0) {
    if (errno == EWOULDBLOCK) {
      // No connections are available; sleep for a short time and try again
      usleep(1000);
      return -1;
    } else {
      // Other error occurred; print an error message and return -1
      error("ERROR: Failed to accept incoming connection");
      return -1;
    }
  }

  // Connection was accepted successfully
  return newsockfd;
}


int srv_send(int sockfd, const void *buf, size_t len, int flags) {
  // Send data to the client
  int bytes_sent = send(sockfd, buf, len, flags);

  // Check for errors
  if (bytes_sent < 0) {
    error("ERROR: Failed to send data to the client");
    return -1;
  }

  return bytes_sent;
}

int srv_recv(int sockfd, void *buf, size_t len, int flags) {
  // Receive data from the client
  int bytes_received = recv(sockfd, buf, len, flags);

  // Check for errors
  if (bytes_received < 0) {
    error("ERROR: Failed to receive data from the client");
    return -1;
  }

  return bytes_received;
}

/*
Reads the entire file into memory to count the number of records and pick a random one.
Not be suitable for very large files. An fseek() version is provided below
// Function to return a random record from a text file
int get_random_record(char* filename, char* buffer) {
  // Seed the random number generator
  srand(time(NULL));

  // Open the file for reading
  FILE* fp = fopen(filename, "r");
  if (!fp) {
    fprintf(stderr, "Error: unable to open file '%s' for reading.\n", filename);
    return 1;
  }

  // Count the number of records in the file
  int num_records = 0;
  char c = fgetc(fp);
  while (c != EOF) {
    if (c == DELIMITER) {
      num_records++;
    }
    c = fgetc(fp);
  }
  rewind(fp);

  // Pick a random record
  int rand_index = rand() % num_records;
  int i = 0;
  while (i <= rand_index) {
    c = fgetc(fp);
    if (c == DELIMITER) {
      i++;
    }
  }

  // Read the record into the buffer
  i = 0;
  c = fgetc(fp);
  while (c != DELIMITER && c != EOF && i < MAX_RECORD_LENGTH - 1) {
    buffer[i] = c;
    c = fgetc(fp);
    i++;
  }
  buffer[i] = '\0';

  // Close the file and return
  fclose(fp);
  return 0;
} */

//Function uses fseek() instead of reading the entire file into memory:
int get_random_record(char* filename, char* bufferx) {

     char *field[4]; // see if the random record is this server, so get another record
     char *dest; //my ip check seems to impact the buffer so it is copied
  // Seed the random number generator
    size_t buffer_len;

    dest = malloc(10);

    srand(time(NULL));

      //public ip
   for (int j = 0; j < 4; j++) {
    field[j] = malloc(MAX_FIELD_LENGTH);
    }

  // Open the file for reading
  FILE* fp = fopen(filename, "r");
  if (!fp) {
    fprintf(stderr, "Error: unable to open file '%s' for reading.\n", filename);
    return 1;
  }

  // Get the file size
  fseek(fp, 0, SEEK_END);
  long file_size = ftell(fp);
  rewind(fp);

//read end of file delimiter, buffer is empty
// Read records until a non-empty one is found
  while (1) {

    // Pick a random position in the file
    long rand_pos = rand() % file_size;
    fseek(fp, rand_pos, SEEK_SET);

    // Skip to the start of the next record
    char c = fgetc(fp);
    while (c != DELIMITER && c != EOF) {
        c = fgetc(fp);
    }

    // Read the record into the buffer
    int i = 0;
    c = fgetc(fp);
    while (c != DELIMITER && c != EOF && i < MAX_RECORD_LENGTH - 1) {
        bufferx[i] = c;
        c = fgetc(fp);
        i++;
    }
    bufferx[i] = '\0';



            // Get the length of the buffer array, including the null terminator
            buffer_len = strlen(bufferx) + 1;

            // Allocate memory for the dest array
            dest = realloc(dest, buffer_len);

            if (dest == NULL) {
                // malloc failed, handle the error
                // ...
            }


     strcpy(dest, bufferx);
     split_record(dest, field);

    //printf("MYIP: %s - BUFFER: %s", my_ip, fields[0]);


    if (strcmp(my_ip, field[0]) == 0) {
        printf("Self connection attempt: %s\n", field[0]);
    } else if (strlen(bufferx) > 3) {
        printf("Valid record is: %s\n", bufferx);
        free(dest);
        break;
    } else {
        printf("Record was empty (read last delimiter)\n");
    }

}

  // Close the file and return
  fclose(fp);
  return 0;
}

int split_record(char* record, char** fields) {
  int num_fields = 0;
  char* token = strtok(record, FIELD_DELIMITER);
  while (token != NULL && num_fields < 4) {
    strncpy(fields[num_fields], token, MAX_FIELD_LENGTH - 1);
    fields[num_fields][MAX_FIELD_LENGTH - 1] = '\0';
    num_fields++;
    token = strtok(NULL, FIELD_DELIMITER);
  }
  return num_fields;
}

// Function to read an entire file into a string
//Send the nodes.db when a client connects to the server
char *read_file_to_string(const char *filename) {
  // Open the file in read-only mode
  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    return NULL; // Error opening the file
  }

  // Move the file pointer to the end of the file
  // to determine the file size
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  rewind(file);

  // Allocate a buffer to hold the file contents
  char *buffer = malloc(file_size + 1);
  if (buffer == NULL) {
    fclose(file);
    return NULL; // Error allocating memory
  }

  // Read the file contents into the buffer
  size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
  if (bytes_read != file_size) {
    free(buffer);
    fclose(file);
    return NULL; // Error reading the file
  }

  // Terminate the string with a null character
  buffer[file_size] = '\0';

  // Close the file and return the buffer
  fclose(file);
  return buffer;
}


// Function that generates a random word
char* generate_word() {
  // Initialize the random number generator with the current time
  srand(time(NULL));

  // Generate a random word length
  int length = rand() % 10 + 1; // Generates a number between 1 and 10

  // Generate the random word
  char* word = malloc(length + 1); // Allocate memory for the word
  for (int i = 0; i < length; i++) {
    // Generate a random character and store it in the array
    word[i] = (char)(rand() % 26 + 'a'); // Generates a lowercase letter
  }
  word[length] = '\0'; // Add a null terminator to the end of the string

  return word; // Return the word
}

char *read_from_socket_and_save_to_file(int sockfd)
{

char* rand_filename = generate_word(); // Generate a random word

    // Open the file in text write mode
    FILE *file = fopen(rand_filename, "w");

    if (file == NULL)
    {
        // Print an error message and exit the program if the file cannot be opened
        fprintf(stderr, "ERROR opening file\n");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    int n;

    // Keep reading from the socket until there is no more data or an error occurs
    while ((n = read(sockfd, buffer, BUFFER_SIZE)) > 0)
    {
        // Write the data from the buffer to the file
        int bytes_written = fwrite(buffer, sizeof(char), n, file);

        // Exit the loop or return from the function if there is an error writing to the file
        if (bytes_written != n)
        {
            fprintf(stderr, "ERROR writing to file\n");
            exit(EXIT_FAILURE);
        }
    }

    // Check for EOF or errors from the read call
    if (n < 0)
    {
        // Print an error message and exit the program if there was an error reading from the socket
        fprintf(stderr, "ERROR reading from socket\n");
        printf("Read returned: %d\n", n);
        printf("Read failed with error: %s\n", strerror(errno));



        exit(EXIT_FAILURE);
    }

    // Close the file
    fclose(file);

  // Free the memory allocated for the word
  return rand_filename;
  //free(rand_filename);
}

// Combine two files into one, no duplicate ip's and ports and ignore local server, do not self connect
// Function to parse a line from the text file and extract the IP address and port number
void parse_line(char *line, record_t *record) {
  // Use strtok() to split the line into tokens using the [ delimiter
  char *token = strtok(line, "[");

  // Use sscanf() to parse the IP address and port number from the first token
  sscanf(token, "%[^,],%d,%[^,],%[^,]", record->ip, &record->port, record->ext1, record->ext2);
}

//Combine nodes.db
// Function to check if a record already exists in the data structure
int record_exists(record_t *records, int num_records, record_t *record) {
  // Check if the record exists in the data structure
  for (int i = 0; i < num_records; i++) {
    if (strcmp(records[i].ip, record->ip) == 0 && records[i].port == record->port) {
      // Record exists, return 1
      return 1;
    }

  }
  // Record does not exist, return 0
  return 0;
}

// Function to combine the records from two input files and write them to an output file
int combine_files(char *file1, char *file2, char *output_file) {
  // Open the first text file in read-only mode
  FILE *f1 = fopen(file1, "r");
  if (f1 == NULL) {
    printf("Error: Unable to open file %s\n", file1);
    return 1;
  }

  // Open the second text file in read-only mode
  FILE *f2 = fopen(file2, "r");
  if (f2 == NULL) {
    printf("Error: Unable to open file %s\n", file2);
    return 1;
  }

  // Open the output file in write mode
  FILE *fout = fopen(output_file, "w");
  if (fout == NULL) {
    printf("Error: Unable to open file %s\n", output_file);
    return 1;
  }

  // Allocate memory for the array of records
  const int max_records = 1000;
  record_t *records = malloc(max_records * sizeof(record_t));
  if (records == NULL) {
    printf("Error: Unable to allocate memory for records\n");
    return 1;
  }

  // Read each line from the input files and parse the IP address and port number
  char line[256];
  int num_records = 0;
  while (fgets(line, sizeof(line), f1) != NULL) {
    // Parse the IP address and port number from the line
    record_t record;
    parse_line(line, &record);

    // Check if the record already exists in the data structure
    if (!record_exists(records, num_records, &record)) {
      // Record does not exist, add it to the data structure
      records[num_records] = record;
      num_records++;
    }
  }

  // Read each line from the second input file and parse the IP address and port number
  while (fgets(line, sizeof(line), f2) != NULL) {
    // Parse the IP address and port number from the line
    record_t record;
    parse_line(line, &record);

    // Check if the record already exists in the data structure
    if (!record_exists(records, num_records, &record)) {
      // Record does not exist, add it to the data structure
      records[num_records] = record;
      num_records++;
    }
  }

  // Write each record to the output file
  for (int i = 0; i < num_records; i++) {
    fprintf(fout, "[%s,%d,%s,%s", records[i].ip, records[i].port, records[i].ext1, records[i].ext2);
  }

  // Close the input and output files
  fclose(f1);
  fclose(f2);
  fclose(fout);

  // Free the memory allocated for the array of records
  free(records);

   // Delete a temp file
  if (delete_temp_file(file2)) {
    printf("Temp file deleted successfully\n");
  } else {
    printf("Error deleting temp file\n");
  }

  // Rename a file
  if (rename_file(output_file, "nodes.db")) {
    printf("File renamed successfully\n");
  } else {
    printf("Error renaming file\n");
  }

  // Return 0 to indicate success
  return 0;
}


// Function to delete a temp file
int delete_temp_file(const char* file_name) {
  int result = remove(file_name);

  if (result == 0) {
    return 1;
  } else {
    return 0;
  }
}

// Function to rename a file
int rename_file(const char* old_name, const char* new_name) {
  int result = rename(old_name, new_name);

  if (result == 0) {
    return 1;
  } else {
    return 0;
  }
}

char *get_public_ip() {
  // Select a random service
  //int rand_num = arc4random_uniform(4); //OpenBSD arc4random.h library
  //const char *hostname = services[rand_num];
char buffer[MYIP_BUFFER];
  // Select a random service
  int rand_num = rand() % 3;
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
  // Check if regular expression is already compiled
  static regex_t regex;
  static int compiled = 0;

  if (!compiled) {
    // Compile regular expression
    int reti = regcomp(&regex, REGEX_PATTERN, REG_EXTENDED);
    if (reti) {
      char msgbuf[100];
      regerror(reti, &regex, msgbuf, sizeof(msgbuf));
      fprintf(stderr, "Could not compile regex: %s\n", msgbuf);
      return NULL;
    }
    compiled = 1;
  }

  // Execute regular expression
  regmatch_t matches[2];
  int reti = regexec(&regex, str, 2, matches, 0);
  if (!reti) {
    // Allocate memory for the IPv4 address
    char *ip = malloc(16);
    memcpy(ip, &str[matches[1].rm_so], matches[1].rm_eo - matches[1].rm_so);
    ip[matches[1].rm_eo - matches[1].rm_so] = '\0';
    return ip;
  } else if (reti == REG_NOMATCH) {
    printf("No match\n");
    return NULL;
  } else {
    char msgbuf[100];
    regerror(reti, &regex, msgbuf, sizeof(msgbuf));
    fprintf(stderr, "Regex match failed: %s\n", msgbuf);
    return NULL;
  }
}


int srv_close(int fd) {
  // Close the socket
  if (close(fd) < 0) {
    error("ERROR: Failed to close the socket");
    return -1;
  }

  return 0;
}

void error(const char *msg) {
  // Print the error message
  perror(msg);
  exit(1);
}

// The signal handler function
void sigintHandler(int sig) {
  // Do something here, like print a message or terminate the program
  printf("Received signal %d\n", sig);

    sigintReceived = 1;

}

int main(void) {
  // Create the socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  pthread_t thread;
  // Initialize the start time
    start_time = clock();

    // Register the signal handler for the SIGINT signal (Ctrl+C)
    signal(SIGINT, sigintHandler);

      //get public ip on start
     srand(time(NULL));  // Seed random number generator

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


  // Check for errors
  if (sockfd < 0) {
    error("ERROR: Failed to create the socket");
    return -1;
  }

  // Bind the socket to a local address
  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(PORT);

  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    error("ERROR: Failed to bind the socket");
    return -1;
  }

  // Set the socket to listen for incoming connections
  if (srv_listen(sockfd, BACKLOG) < 0) {
    error("ERROR: Failed to listen on the socket");
    return -1;
  }

  // Create the new thread for a connect routine
  int result = pthread_create(&thread, NULL, thread_function, NULL);
  if (result != 0) {
    printf("Error creating thread\n");
    return -1;
  }

  // Enter the main loop
  while (!sigintReceived) {

    //Check public ip everyday
    //Check if the delay time has passed
     /*   current_time = clock();
        if ((current_time - start_time) / CLOCKS_PER_SEC >= DELAY_TIME) {

            srand(time(NULL));  // Seed random number generator

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

    //    printf("Public IP: %s\n", ip);
    //    free(ip);
        free(response);

             // Reset the start time
            start_time = current_time;
    } */


    // Accept an incoming connection
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    int newsockfd = srv_accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

    if (newsockfd > -1) {
    // Receive data from the client
    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);
    int bytes_received = srv_recv(newsockfd, buffer, BUFFER_SIZE, 0);
    printf("Recv Message: %s\n", buffer);

    // Check for errors
    if (bytes_received < 0) {
      error("ERROR: Failed to receive data from the client");
      continue;
    }

    // Send a response to the client

    char *contents = read_file_to_string("nodes.db");
    if (contents == NULL) {
    // Handle error
    }


    //char response[] = contents; //"Hello, world!";
    int bytes_sent = srv_send(newsockfd, contents, strlen(contents), 0);
    printf("Send Message: %s\n", contents);

    //Check for errors
    if (bytes_sent < 0) {
    perror("ERROR: Failed to send data to the server");
    return 1;
    }

        // Close the client socket
        srv_close(newsockfd);
        // Don't forget to free the buffer when you're done with it
        free(contents); //ip.txt in mem

    }
  }

    //End the connect thread, end the server socket
   // Set the flag to indicate that the thread should end
  thread_should_end = true;

  // Wait for the thread to finish
  result = pthread_join(thread, NULL);
  if (result != 0) {
    printf("Error joining thread\n");
    return -1;
  }

    // Close the server socket
    srv_close(sockfd);

        printf("Server shutdown finished\n");

    return 0;
}
