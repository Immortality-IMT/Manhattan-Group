#include "functions.h"

//gcc -g -o wallet transactions.c wallet.c verifications.c functions.h miner.c blockchain.c -lssl -lcrypto -lsqlite3 -lz

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

    char *create_table_query = "CREATE TABLE IF NOT EXISTS transactions (txid TEXT PRIMARY KEY, sender TEXT, recipient TEXT, amount REAL, miners_fee REAL, moonshot_fee REAL, data TEXT, data_type INTEGER, timestamp DATETIME, r_hex_signature TEXT, s_hex_signature TEXT, confirmed INTEGER);";

    rc = sqlite3_exec(db, create_table_query, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }

    printf("Enter sender address: "); getchar();
    fgets(new_transaction.sender, sizeof(new_transaction.sender), stdin);

    printf("Enter recipient address: "); getchar();
    fgets(new_transaction.recipient, sizeof(new_transaction.recipient), stdin);

    printf("Enter amount: ");
    scanf("%lf", &new_transaction.amount);

    printf("Enter transaction fee: ");
    scanf("%lf", &new_transaction.miners_fee);

    printf("Enter moonshot fee: ");
    scanf("%lf", &new_transaction.moonshot_fee);

    printf("Enter data (smart contract or a text description of the transaction) : "); getchar();
    fgets(new_transaction.data, sizeof(new_transaction.data), stdin);

    /*
    char *data = NULL;
    size_t data_len = 0;
    printf("Enter data (smart contract or a text description of the transaction) : "); getchar();
    getline(&data, &data_len, stdin);
    data_len = strlen(data);
    if (data[data_len-1] == '\n') {
        data[data_len-1] = '\0';
    }*/

    printf("What is this data? (1 text, 2 smart contract, 3 image...) : ");
    scanf("%d", &new_transaction.data_type);

    new_transaction.timestamp = time(NULL);

    //new_transaction.confirmed = 0;

/* Can compress the data field to huffman encoding

    char *compressed_data = (char *) malloc(CHUNK);
    int compressed_len = 0;
    int ret = compress_data(data, data_len, &compressed_data, &compressed_len);
    if (ret != 0) {
        printf("Failed to compress data\n");
        return 1;
    }

    char *decompressed_data = (char *) malloc(CHUNK);
    int decompressed_data_len = 0;
    ret = decompress_data(compressed_data, compressed_len, &decompressed_data, &decompressed_data_len);
    if (ret != 0) {
        printf("Failed to decompress data\n");
        return 1;
    }

    printf("Original data: %s\n", data);
    printf("Compressed data: %s\n", compressed_data);
    printf("Decompressed data: %s\n", decompressed_data);
*/
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
    snprintf(sql, sizeof(sql), "SELECT private_key FROM wallet WHERE address = %s;", new_transaction.sender);
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

    int ret = snprintf(sql, sizeof(sql), "INSERT INTO transactions (txid, sender, recipient, amount, miners_fee, moonshot_fee, data, data_type, timestamp, r_hex_signature, s_hex_signature, confirmed) VALUES ('%s', '%s', '%s', %f, %f, %f, '%s', '%d', %ld, '%s', '%s', '%d');", new_transaction.txid, new_transaction.sender, new_transaction.recipient, new_transaction.amount, new_transaction.miners_fee, new_transaction.moonshot_fee, new_transaction.data, new_transaction.data_type, new_transaction.timestamp, new_transaction.r_hex_signature, new_transaction.s_hex_signature, new_transaction.confirmed);

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

    //free(compressed_data);
    //free(decompressed_data);
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
Compress data, used for the transaction variable data[1mB] which holds any info, text, iamges and including smart_contract data.
*/

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
