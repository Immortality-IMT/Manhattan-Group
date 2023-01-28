#include "functions.h"

/*
    Grab transactions from the transaction pool (transactions.db) and make a block
    gcc -g -o wallet transactions.c wallet.c verifications.c functions.h miner.c blockchain.c -lssl -lcrypto -lsqlite3 -lz
*/

//void create_block() {
struct block create_block(void) { //actually returns a block

    sqlite3 *db;
    char *zErrMsg = 0;
    char **result;
    int nrow, ncol;
    char *output = malloc(1);

    int rc = sqlite3_open(DB_TRANSACTIONS, &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
    }

    char *sql = "SELECT * FROM transactions";
    rc = sqlite3_get_table(db, sql, &result, &nrow, &ncol, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        int i, j;
        output[0] = '\0';
        for (i = 1; i <= nrow; i++) {
            for (j = 0; j < ncol; j++) {
                int len = strlen(output) + strlen(result[i*ncol+j]) + 2;
                output = realloc(output, len);
                strcat(output, result[i*ncol+j]);
                strcat(output, ",");
            }
            int len = strlen(output) + 2;
            output = realloc(output, len);
            strcat(output, "\n");
        }

        //printf(">>>>>>>>>: %s", output);
    }

    sqlite3_free_table(result);
    sqlite3_close(db);

    struct block new_block;
    new_block.timestamp = time(NULL);
    strcpy(new_block.transaction_bundle, output);
    strcpy(new_block.previous_hash, get_previous_hash());
    hash_block(&new_block);

       free(output);

    return new_block;
}

char* get_previous_hash(void) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc = sqlite3_open(DB_BLOCKCHAIN, &db);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }

    char *sql = "SELECT block_hash FROM blocks ORDER BY timestamp DESC LIMIT 1";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

    if (rc != SQLITE_OK ) {
        fprintf(stderr, "Failed to select data\n");
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        exit(1);
    }

    rc = sqlite3_step(stmt);
    char *previous_hash = strdup((char*)sqlite3_column_text(stmt, 0));

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return previous_hash;
}

void hash_block(struct block *b) {
    // Create a SHA-256 context
    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    // Hash the previous hash, the transaction bundle, and the timestamp
    SHA256_Update(&ctx, b->previous_hash, sizeof(b->previous_hash));
    SHA256_Update(&ctx, b->transaction_bundle, sizeof(b->transaction_bundle));
    SHA256_Update(&ctx, &b->timestamp, sizeof(b->timestamp));

    // Finalize the hash and store it in the block's block_hash field
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &ctx);

    // Create a BIO for hexadecimal conversion
    BIO *bio, *b64;
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, hash, sizeof(hash));
    BIO_flush(bio);

    // Get the hexadecimal string from the BIO
    char *hex_str;
    int len = BIO_get_mem_data(bio, &hex_str);
    memcpy(b->block_hash, hex_str, len);
    b->block_hash[len] = '\0';

    // Clean up
    BIO_free_all(bio);
}

/*
void hash_block(struct block *b) {
    // Create a SHA-256 context
    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    // Hash the previous hash, the transaction bundle, and the timestamp
    SHA256_Update(&ctx, b->previous_hash, sizeof(b->previous_hash));
    SHA256_Update(&ctx, b->transaction_bundle, sizeof(b->transaction_bundle));
    SHA256_Update(&ctx, &b->timestamp, sizeof(b->timestamp));

    // Finalize the hash and store it in the block's block_hash field
    SHA256_Final((unsigned char *) b->block_hash, &ctx);

    printf(">:%s", b->block_hash);

}*/

void add_block(struct block new_block) {

        printf("\nThe Block\n");
        printf("Block Hash: %s\n", new_block.block_hash);
        printf("Previous Hash: %s\n", new_block.previous_hash);
        printf("Transactions: %s\n", new_block.transaction_bundle);
        printf("Timestamp: %d\n", new_block.timestamp);
        printf("\nSize of new_block struct: %lu\n", sizeof(new_block));

    sqlite3 *db;
    char *err_msg = 0;
    int rc = sqlite3_open(DB_BLOCKCHAIN, &db);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        //exit(1);
    }

    char *sql = "INSERT INTO blocks (block_hash, previous_hash, block_data, timestamp) VALUES (?, ?, ?, ?);";

    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

    if (rc != SQLITE_OK ) {
        fprintf(stderr, "Failed to insert data\n");
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        exit(1);
    }

    sqlite3_bind_text(stmt, 1, new_block.block_hash, strlen(new_block.block_hash), SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, new_block.previous_hash, strlen(new_block.previous_hash), SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, new_block.transaction_bundle, strlen(new_block.transaction_bundle), SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, new_block.timestamp);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to insert data\n");
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

/* mine */
int mine() {
   
    start_blockchain(); //generate a new blockchain db

    struct block new_block = create_block();
    add_block(new_block);
    printf("Block added to blockchain: %s\n", new_block.block_hash);
    return 0;

}
