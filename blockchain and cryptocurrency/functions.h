#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/rand.h>
#include <openssl/ec.h>
#include <openssl/objects.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <openssl/ecdsa.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <sqlite3.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <time.h>
#include <zlib.h>

#define CHUNK 16384

#define BLOCK_DATA_SIZE 1024

#define DB_WALLET "DB/wallet.db"
#define DB_TRANSACTIONS "DB/transactions.db"
#define DB_BLOCKCHAIN "DB/blockchain.db"

struct wallet {
    char private_key_b64[45]; //secret key used to ensure your identity
    char public_key_b64[65]; //use address instead
    char address[45]; //what you send to get paid or to pay
    char statement[256]; //bank statement of past transactions
    char balance[65]; //how much money you have
};

// Definition of a transaction that becomes part of a block
struct transaction {
    char sender[45]; //44 char address
    char recipient[45]; //44 char address
    double amount;
    double miners_fee;
    double moonshot_fee;
    char data[1048577]; //general data variable, bytecode for smart_contracts, journal paper, book, text message, (zlib compressed)
    int data_type;
    time_t timestamp;
    char r_hex_signature[1024];
    char s_hex_signature[1024];
    //char input_transactions[50][256]; //a transaction can include multiple inputs, each of which is the output of a previous transaction.
    int confirmed; // the status of the transaction
    char txid[65]; //64-character hexadecimal string calculated using the SHA-256 cryptographic hash function.
};

// Definition of a block that goes on a blockchain
struct block {
    char previous_hash[256];
    char block_hash[256];
    char transaction_bundle[8192];
    int timestamp;
};

// Block reward
struct coinbase_transaction {
    int block_height;
    double block_reward;
    double moonshot_reward;
    char miner_address[128];
    char coinbase_data[4096];
    time_t timestamp;
    char r_hex_signature[1024];
    char s_hex_signature[1024];
};

//wallet.c
BIGNUM* private_key();
unsigned char* public_key(BIGNUM* private_key);
char* public_key_to_hex(unsigned char* public_key);
char* public_key_to_address(unsigned char* public_key);
void get_keys(BIGNUM* private_key_ptr,unsigned char* public_key_ptr, char* private_key_b64, char* public_key_b64, char* address);
int generate_address();
void print_table(const char* db_name, const char* table_name);
static int callback(void *data, int argc, char **argv, char **col_name);
int main();

//transactions.c
int create_transaction();
void sign_transaction(const char *sender_address, const char *receiver_address, int amount, const char *private_key_b64, char **r_hex, char **s_hex);
void get_txid(struct transaction* tx);
int compress_data(const char *data, int data_len, char **compressed_data, int *compressed_len);
int decompress_data(const char *compressed_data, int compressed_len, char **decompressed_data, int *decompressed_data_len);

//miner.c
struct block create_block(void);
char* get_previous_hash(void);
void hash_block(struct block *b);
void add_block(struct block new_block);
int mine();

//blockchain.c
static int callback_count(void *transaction_bundle, int argc, char **argv, char **azColName);
int start_blockchain();

//verifications.c
int verify_transaction();
int verify_block();
int verify_chain();

