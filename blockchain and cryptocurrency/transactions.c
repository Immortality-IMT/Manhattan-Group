#include "functions.h"

int create_transaction() {

    struct transaction new_transaction;

    sqlite3 *db;
    char *err_msg = 0;
    int rc;
    const char *private_key_b64;
    char sql[2048];

    rc = sqlite3_open(DB_TRANSACTIONS, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    char *create_table_query = "CREATE TABLE IF NOT EXISTS transactions (txid TEXT PRIMARY KEY, sender TEXT, recipient TEXT, amount REAL, miners_fee REAL, moonshot_fee REAL, data TEXT, data_type INTEGER, timestamp DATETIME, r_hex_signature TEXT, s_hex_signature TEXT);";

//    char *create_table_query = "CREATE TABLE IF NOT EXISTS transactions (txid TEXT PRIMARY KEY, sender TEXT, recipient TEXT, amount REAL, miners_fee REAL, moonshot_fee REAL, data TEXT, timestamp DATETIME, r_hex_signature TEXT, s_hex_signature TEXT);";
    rc = sqlite3_exec(db, create_table_query, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }

    printf("Enter sender address: ");
    scanf("%127s", new_transaction.sender);

    printf("Enter recipient address: ");
    scanf("%127s", new_transaction.recipient);

    printf("Enter amount: ");
    scanf("%lf", &new_transaction.amount);

    printf("Enter transaction fee: ");
    scanf("%lf", &new_transaction.miners_fee);

    printf("Enter moonshot fee: ");
    scanf("%lf", &new_transaction.moonshot_fee);

    printf("Enter data (smart contract or a text description of the transaction) : ");
    scanf("%s", new_transaction.data);

    printf("What is this data? (1 text, 2 smart contract, 3 image...) : ");
    scanf("%d", &new_transaction.data_type);

    new_transaction.timestamp = time(NULL);

/*
    struct tm* timeinfo = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
*/

/* Check local blockchain for valid transaction */
    /* 
       Do a verification of blockchain.db to determine if transaction is valid 
       Does the sender have the funds
    */

/* Sign the transaction with senders private key */
    rc = sqlite3_open(DB_WALLET, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error opening database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }
    snprintf(sql, sizeof(sql), "SELECT private_key_b64 FROM wallet WHERE address = %s;", new_transaction.sender);
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, new_transaction.sender, -1, SQLITE_STATIC);
    int result = sqlite3_step(stmt);
    if (result == SQLITE_ROW) {
        private_key_b64 = (const char *)sqlite3_column_text(stmt, 0);
    }
    sqlite3_finalize(stmt);
    
    char *r_hex, *s_hex;
    sign_transaction(new_transaction.sender, new_transaction.recipient, new_transaction.amount, private_key_b64, &r_hex, &s_hex); //sign it
    printf("Signature: r_hex: %s, s_hex: %s\n", r_hex, s_hex);

    strcpy(new_transaction.r_hex_signature, r_hex);
    strcpy(new_transaction.s_hex_signature, s_hex);

    //Determine the txtid, double sha
    get_txid(&new_transaction);
    printf("Transaction ID: %s\n", new_transaction.txid);

/* Put into the transaction pool, send it, broadcast to the network, they perform verification and block it up... */

    int ret = snprintf(sql, sizeof(sql), "INSERT INTO transactions (txid, sender, recipient, amount, miners_fee, moonshot_fee, data, data_type, timestamp, r_hex_signature, s_hex_signature) VALUES ('%s', '%s', '%s', %f, %f, %f, '%s', '%d', %ld, '%s', '%s');", new_transaction.txid, new_transaction.sender, new_transaction.recipient, new_transaction.amount, new_transaction.miners_fee, new_transaction.moonshot_fee, new_transaction.data, new_transaction.data_type, new_transaction.timestamp, new_transaction.r_hex_signature, new_transaction.s_hex_signature);

    rc = sqlite3_open(DB_TRANSACTIONS, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error opening database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK ) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }

    sqlite3_close(db);
}

void sign_transaction(const char *sender_address, const char *receiver_address, int amount, const char *private_key_b64, char **r_hex, char **s_hex) {
    //Decode the private key from base64
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bmem = BIO_new_mem_buf(private_key_b64, -1);
    bmem = BIO_push(b64, bmem);
    unsigned char private_key[256];
    int private_key_len = BIO_read(bmem, private_key, sizeof(private_key));

    EC_KEY *key = EC_KEY_new_by_curve_name(NID_secp256k1);
    BIGNUM *private_key_bn = BN_new();
    BN_bin2bn(private_key, private_key_len, private_key_bn);
    EC_KEY_set_private_key(key, private_key_bn);

    //Build the message to sign
    char message[256];
    sprintf(message, "%s:%s:%d", sender_address, receiver_address, amount);

    //Sign the message
    ECDSA_SIG *signature = ECDSA_do_sign((unsigned char *)message, strlen(message), key);
 
    //Convert the signature to hex
    *r_hex = BN_bn2hex(ECDSA_SIG_get0_r(signature));
    *s_hex = BN_bn2hex(ECDSA_SIG_get0_s(signature));

    //Free memory
    BN_free(private_key_bn);
    EC_KEY_free(key);
    ECDSA_SIG_free(signature);
    BIO_free_all(bmem);

    //Return the hex signature
    //printf("Sign Signature: %s:%s\n", *r_hex, *s_hex);
}

/*
void get_txid(struct transaction* tx) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    // initialize the SHA256 context
    SHA256_Init(&sha256);
    // update the context with the variables of the struct
    size_t sender_size = strlen(tx->sender);
    SHA256_Update(&sha256, tx->sender, sender_size);
    size_t recipient_size = strlen(tx->recipient);
    SHA256_Update(&sha256, tx->recipient, recipient_size);
    size_t amount_size = sizeof(tx->amount);
    SHA256_Update(&sha256, &tx->amount, amount_size);
    size_t timestamp_size = sizeof(tx->timestamp);
    SHA256_Update(&sha256, &tx->timestamp, timestamp_size);
    // finalize the hash
    SHA256_Final(hash, &sha256);
    // double SHA-256
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, hash, SHA256_DIGEST_LENGTH);
    SHA256_Final(hash, &sha256);
    int i;
    for(i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    sprintf(tx->txid + (i * 2), "%02x", hash[i]);
    }
}*/


void get_txid(struct transaction* tx) {

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, (unsigned char*)tx, sizeof(struct transaction));
    SHA256_Final(hash, &sha256);
    //double SHA-256
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, hash, SHA256_DIGEST_LENGTH);
    SHA256_Final(hash, &sha256);
    int i;

    for(i = 0; i < SHA256_DIGEST_LENGTH; i++)
        sprintf(tx->txid + (i * 2), "%02x", hash[i]);
}

/*
void get_txid(struct transaction* tx) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, (unsigned char*)tx->sender, strlen(tx->sender));
    SHA256_Update(&sha256, (unsigned char*)tx->recipient, strlen(tx->recipient));
    SHA256_Update(&sha256, (unsigned char*)&tx->amount, sizeof(tx->amount));
    SHA256_Update(&sha256, (unsigned char*)&tx->timestamp, sizeof(tx->timestamp));
    SHA256_Final(hash, &sha256);
    //double SHA-256
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, hash, SHA256_DIGEST_LENGTH);
    SHA256_Final(hash, &sha256);
    int i;
    for(i = 0; i < SHA256_DIGEST_LENGTH; i++)
        sprintf(tx->txid + (i * 2), "%02x", hash[i]);
}
*/


/*
Compress data, used for the transaction variable data[1mB] which holds any info, text, iamges and including smart_contract data.

char data[] = "Hello World!";
int data_len = sizeof(data);

char compressed_data[100];
int compressed_len;

compress_data(data, data_len, compressed_data, &compressed_len);

char decompressed_data[100];
int decompressed_data_len;

decompress_data(compressed_data, compressed_len, decompressed_data, &decompressed_data_len);
*/

int compress_data(const char *data, int data_len, char *compressed_data, int *compressed_len) {
    int ret;
    z_stream strm;

    // Initialize the zlib stream
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK) {
        return ret;
    }

    // Compress the data
    strm.avail_in = data_len;
    strm.next_in = (unsigned char *) data;
    strm.avail_out = *compressed_len;
    strm.next_out = (unsigned char *) compressed_data;
    ret = deflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        deflateEnd(&strm);
        return ret;
    }

    // Update the compressed data length
    *compressed_len = strm.total_out;

    // Clean up
    deflateEnd(&strm);
    return Z_OK;
}

// Decompress data
int decompress_data(const char *compressed_data, int compressed_len, char *data, int *data_len) {
    int ret;
    z_stream strm;

    // Initialize the zlib stream
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = compressed_len;
    strm.next_in = (unsigned char *) compressed_data;
    ret = inflateInit(&strm);
    if (ret != Z_OK) {
        return ret;
    }

    // Decompress the data
    strm.avail_out = *data_len;
    strm.next_out = (unsigned char *) data;
    ret = inflate(&strm, Z_NO_FLUSH);
    if (ret != Z_STREAM_END) {
        inflateEnd(&strm);
        return ret;
    }

    // Update the decompressed data length
    *data_len = strm.total_out;

    // Clean up
    inflateEnd(&strm);
    return Z_OK;
}
