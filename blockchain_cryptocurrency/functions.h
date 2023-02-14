#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/buffer.h>
#include <openssl/crypto.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/ssl.h>
#include <sqlite3.h>
#include <zlib.h>
#include <oqs/oqs.h>

#define CHUNK 16384

#define BLOCK_DATA_SIZE 1024

#define MAX_TRANSACTIONS 100
#define BLOCK_SIZE 1000000
#define DIFFICULTY 3              //the higher the more processing power, the harder

#define DB_WALLET "DB/wallet.db"
#define DB_TRANSACTIONS "DB/transactions.db"
#define DB_BLOCKCHAIN "DB/blockchain.db"

#define OQS_SIG_dilithium_2_length_public_key_hex (1312 * 2) + 1
#define OQS_SIG_dilithium_2_length_secret_key_hex (2528 * 2) + 1
#define OQS_SIG_dilithium_2_length_signature_hex (2420 * 2) + 1

struct enc_wallet {
    uint8_t public_key[OQS_SIG_dilithium_2_length_public_key_hex]; //public key always 1312 in Dilithium2, EUF-CMA, 2 x hex encoded + 1 \0
    uint8_t private_key[OQS_SIG_dilithium_2_length_secret_key_hex]; //private key always 2528 in Dilithium2, EUF-CMA, 2 x hex encoded, use address instead
    uint8_t address[72]; //address is sha3-384, 48 bytes = 384 bits, what you send to get pay or pay //base58 enc usually 66 bytes, 72 to be sure
    //uint8_t signature[4840]; //sinature is always 2420 in Dilithium2, EUF-CMA, 2 x hex encoded
    char statement[256]; //bank statement of past transactions
    char balance[32]; //how much money in the account
};

struct bin_wallet {
    uint8_t public_key[OQS_SIG_dilithium_2_length_public_key]; //public key is always 1312 in Dilithium2, EUF-CMA, secret hex encoded key used to secure funds
    uint8_t private_key[OQS_SIG_dilithium_2_length_secret_key]; //private key is always 2528 in Dilithium2, EUF-CMA, hex encoded, use address instead
    uint8_t address[48]; //address is sha3-384, 48 bytes = 384 bits, what you send to get pay or pay //base58 enc 96 bytes
    //uint8_t signature[2420]; //sinature is always 2420 in Dilithium2, EUF-CMA
    char statement[256]; //bank statement of past transactions
    char balance[32]; //how much money in the account
};

// Definition of a transaction that becomes part of a block
struct transaction {
    char sender[72]; //44 char address
    char recipient[72]; //44 char address
    long double amount;
    long double miners_fee;
    long double moonshot_fee;
    char data[1024]; //general data variable, bytecode for smart_contracts, journal paper, book, text message, (zlib compressed)
    int data_type;
    time_t timestamp;
    uint8_t signature[OQS_SIG_dilithium_2_length_signature_hex];  //2420, not hex encoded, * 2 hex encoded + 1 null terminator
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
    int64_t block_reward;
    int64_t moonshot_reward;
    char miner_address[128];
    char coinbase_data[4096];
    time_t timestamp;
    char r_hex_signature[1024];
    char s_hex_signature[1024];
};

//wallet.c
int generate_key_pair(uint8_t *public_key, uint8_t *private_key);
int generate_new_address(uint8_t *public_key, uint8_t *address);
int generate_address();  //Generate 5 pub/priv/address and store then in the wallet.db for use
void print_table(const char* db_name, const char* table_name);
static int callback(void *data, int argc, char **argv, char **col_name);
int main();

//transactions.c
int create_transaction();
int sign_transaction(uint8_t *sender_address, uint8_t *receiver_address, long double amount, uint8_t private_key[5056], uint8_t public_key[2624], uint8_t *signature);
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
void cleanup_stack(uint8_t *secret_key, size_t secret_key_len);
int validate_key_pair(uint8_t *public_key, uint8_t *private_key, size_t public_key_length, size_t private_key_length); //wallet, good keypair check
int test_keys_in_wallet();

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

*/
