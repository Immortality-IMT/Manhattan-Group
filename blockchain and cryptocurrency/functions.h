#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <memory.h>
#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/buffer.h>
#include <openssl/crypto.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/rand.h>
#include <openssl/ripemd.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>
#include <sqlite3.h>
#include <time.h>
#include <zlib.h>
#include <oqs/oqs.h>
#include <openssl/err.h>

#define CHUNK 16384

#define BLOCK_DATA_SIZE 1024

#define MAX_TRANSACTIONS 100
#define BLOCK_SIZE 1000000
#define DIFFICULTY 2              //the higher the more processing power, the harder

#define DB_WALLET "DB/wallet.db"
#define DB_TRANSACTIONS "DB/transactions.db"
#define DB_BLOCKCHAIN "DB/blockchain.db"

struct enc_wallet {
    uint8_t public_key[2624]; //public key is always 1312 in Dilithium2, EUF-CMA, 2 x secret hex encoded key used to secure funds
    uint8_t private_key[5056]; //private key is always 2528 in Dilithium2, EUF-CMA, 2 x hex encoded, use address instead
    uint8_t address[96]; //address is sha3-384, 48 bytes = 384 bits, what you send to get pay or pay //base58 enc usually 66 bytes, 96 to be sure
    //uint8_t signature[4840]; //sinature is always 2420 in Dilithium2, EUF-CMA, 2 x hex encoded
    char statement[256]; //bank statement of past transactions
    char balance[32]; //how much money in the account
};

struct bin_wallet {
    uint8_t public_key[1312]; //public key is always 1312 in Dilithium2, EUF-CMA, secret hex encoded key used to secure funds
    uint8_t private_key[2528]; //private key is always 2528 in Dilithium2, EUF-CMA, hex encoded, use address instead
    uint8_t address[48]; //address is sha3-384, 48 bytes = 384 bits, what you send to get pay or pay //base58 enc 96 bytes
    //uint8_t signature[2420]; //sinature is always 2420 in Dilithium2, EUF-CMA
    char statement[256]; //bank statement of past transactions
    char balance[32]; //how much money in the account
};

// Definition of a transaction that becomes part of a block
struct transaction {
    char sender[96]; //44 char address
    char recipient[96]; //44 char address
    double amount;
    double miners_fee;
    double moonshot_fee;
    char data[1024]; //general data variable, bytecode for smart_contracts, journal paper, book, text message, (zlib compressed)
    int data_type;
    time_t timestamp;
    uint8_t signature[2420];
    //char input_transactions[50][256]; //a transaction can include multiple inputs, each of which is the output of a previous transaction.
    int confirmed; // the status of the transaction
    char txid[96]; //Sha3 - 384bit is 48 bytes x 2 for hexadecimal ecoding = 96, calculated using the SHA3-384 cryptographic hash function.
};

// Definition of a block that goes on a blockchain
struct block {
    int block_index;
    char previous_hash[96];
    char block_hash[48];
    char block_hash_hex[128];
    char transaction_bundle[19999];
    int transaction_count;
    time_t timestamp;
    int nonce;
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
void generate_new_address(uint8_t *public_key, uint8_t *address);
//void generate_new_address(const unsigned char *public_key, unsigned char *address);
void private_key_to_hex(BIGNUM *private_key_ptr, char hex_private_key[66]);
void public_key_to_hex(unsigned char* public_key, char hex_public_key[66]);
int generate_address();
void print_table(const char* db_name, const char* table_name);
static int callback(void *data, int argc, char **argv, char **col_name);
int main();

char* public_key_to_address(unsigned char* public_key, int public_key_len);

//transactions.c
int create_transaction();
void sign_transaction(uint8_t *sender_address, uint8_t *receiver_address, double amount, uint8_t private_key[5056], uint8_t public_key[2624], uint8_t *signature);
//void sign_transaction(const char *sender_address, const char *receiver_address, int amount, const char *private_key_b64, char **r_hex, char **s_hex);
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
int validate_transaction(struct transaction *t);
int validate_block(struct block *b, int difficulty);
int validate_blockchain(struct block *blocks, int num_blocks);

//base58.c
extern bool (*b58_sha256_impl)(void *, const void *, size_t);

extern bool b58tobin(void *bin, size_t *binsz, const char *b58, size_t b58sz);
extern int b58check(const void *bin, size_t binsz, const char *b58, size_t b58sz);

extern bool b58enc(char *b58, size_t *b58sz, const void *bin, size_t binsz);
extern bool b58check_enc(char *b58c, size_t *b58c_sz, uint8_t ver, const void *data, size_t datasz);

/* Bin for possibly useful functions store then here, as many as you like

//Compress data, used for the transaction variable data[1mB] which holds any info, text, iamges and including smart_contract data.
int compress_data(const char *data, int data_len, char **compressed_data, int *compressed_len) {
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    deflateInit(&strm, Z_DEFAULT_COMPRESSION);

    strm.avail_in = data_len;
    strm.next_in = (unsigned char *) data;

    strm.avail_out = CHUNK;
    strm.next_out = (unsigned char *) *compressed_data;

    deflate(&strm, Z_FINISH);
    *compressed_len = strm.total_out;
    deflateEnd(&strm);
    return 0;
}

int decompress_data(const char *compressed_data, int compressed_len, char **decompressed_data, int *decompressed_data_len) {
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    inflateInit(&strm);

    strm.avail_in = compressed_len;
    strm.next_in = (unsigned char *) compressed_data;

    strm.avail_out = CHUNK;
    strm.next_out = (unsigned char *) *decompressed_data;

    inflate(&strm, Z_NO_FLUSH);
    *decompressed_data_len = strm.total_out;
    inflateEnd(&strm);
    return 0;
}

//base 64 encode and decode
void base64_encode(const uint8_t *private_key, char **output) {
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);

    BIO_write(b64, private_key, 2528);
    BIO_flush(b64);

    BUF_MEM *bptr;
    BIO_get_mem_ptr(b64, &bptr);

    *output = (char *)malloc(bptr->length);
    memcpy(*output, bptr->data, bptr->length-1);
    (*output)[bptr->length-1] = 0;

    BIO_free_all(b64);
}

void base64_decode(char *input, uint8_t **output, int *len)
{
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bmem = BIO_new_mem_buf(input, -1);
    bmem = BIO_push(b64, bmem);

    *output = (uint8_t *)malloc(2528);
    *len = BIO_read(bmem, *output, 2528);

    BIO_free_all(bmem);
}

int ....()
{
    uint8_t private_key[2528];
    // Fill private_key with binary data

    char *encoded_key;
    base64_encode(private_key, &encoded_key);
    printf("Encoded Key: %s\n", encoded_key);

    uint8_t *decoded_key;
    int decoded_len;
    base64_decode(encoded_key, &decoded_key, &decoded_len);
    printf("Decoded Length: %d\n", decoded_len);

    return 0;
}

//

*/
