#include "functions.h"

/*
    Grab transactions from the transaction pool (transactions.db) and make a block
    gcc -g -o wallet transactions.c wallet.c verifications.c functions.h miner.c blockchain.c -lssl -lcrypto -lsqlite3 -lz
*/

struct block create_block(void) { //actually returns a block

    sqlite3 *db;
    char *zErrMsg = 0;
    char **result;
    int nrow, ncol;
    char *output = malloc(1);
    int counter = 0;

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
            counter++;
        }

        printf("COUNTER: %d", counter);
    }

    sqlite3_free_table(result);
    sqlite3_close(db);

    struct block new_block;
    new_block.timestamp = time(NULL);
    new_block.transaction_count = 0;
    new_block.nonce = 0;

    strcpy(new_block.transaction_bundle, output);
    strcpy(new_block.previous_hash, get_previous_hash());

    new_block.transaction_count = counter;

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

// change PoW DIFFICULTY in function.h
void hash_block(struct block *b) {
    char data[129999];
    sprintf(data, "%d%s%ld%d%s", b->block_index, b->previous_hash, b->timestamp, b->transaction_count, b->transaction_bundle);

    unsigned char hash[SHA384_DIGEST_LENGTH];
    SHA384((unsigned char *)data, strlen(data), hash);

    int nonce = 0;
    int leading_zeros = 0;

    while (nonce < UINT32_MAX) {
        unsigned char new_hash[SHA384_DIGEST_LENGTH];
        sprintf(data, "%d%s%ld%d%s%d", b->block_index, b->previous_hash, b->timestamp, b->transaction_count, b->transaction_bundle, nonce);
        SHA384((unsigned char *)data, strlen(data), new_hash);

        leading_zeros = 0;
        for (int i = 0; i < DIFFICULTY; i++) {
            if (new_hash[i] == 0) {
                leading_zeros++;
            } else {
                break;
            }
        }

        if (leading_zeros == DIFFICULTY) {
            memcpy(b->block_hash, new_hash, SHA384_DIGEST_LENGTH);
            b->nonce = nonce;
            break;
        }

        nonce++;

        // Debug information
        if (nonce % 1000 == 0) {
            printf("Current nonce: %d\n", nonce);
            printf("Leading zeros: %d\n", leading_zeros);
        }
    }

    printf("Proof-of-work successful!\n");
    printf("Final nonce: %d\n", b->nonce);


    for (int i = 0; i < 48; i++) {
        sprintf(b->block_hash_hex + (i * 2), "%02x",  b->block_hash[i]);
    }

    printf("\n>>>>%s\n", b->block_hash_hex);
    printf("\n");

    printf("Tranaction Data: %s", b->transaction_bundle);

}

/*
void hash_block(struct block *b) {
    // Create a SHA3 384-bit context
    EVP_MD_CTX *mdctx;
    const EVP_MD *md;
    unsigned char md_value[EVP_MAX_MD_SIZE];
    int md_len, i;

    mdctx = EVP_MD_CTX_new();
    md = EVP_sha3_384();
    EVP_DigestInit_ex(mdctx, md, NULL);

    // Hash the previous hash, the transaction bundle, and the timestamp
    EVP_DigestUpdate(mdctx, b->previous_hash, sizeof(b->previous_hash));
    EVP_DigestUpdate(mdctx, b->transaction_bundle, sizeof(b->transaction_bundle));
    EVP_DigestUpdate(mdctx, &b->timestamp, sizeof(b->timestamp));

    // Finalize the hash and store it in the block's block_hash field
    EVP_DigestFinal_ex(mdctx, md_value, &md_len);

    for (i = 0; i < md_len; i++) {
        sprintf(b->block_hash + (i * 2), "%02x", md_value[i]);
    }
    b->block_hash[md_len * 2] = '\0';

    // Clean up
    EVP_MD_CTX_free(mdctx);
}
*/

void add_block(struct block new_block) {

    printf("\nThe Block\n");
    printf("Block Hash: %s\n", new_block.block_hash_hex);
    printf("Previous Hash: %s\n", new_block.previous_hash);
    printf("Transactions: %s\n", new_block.transaction_bundle);
    printf("Timestamp: %ld\n", new_block.timestamp);
    printf("\nSize of new_block struct: %lu\n", sizeof(new_block));

    sqlite3 *db;
    char *err_msg = 0;
    int rc = sqlite3_open(DB_BLOCKCHAIN, &db);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        //exit(1);
    }

    char *sql = "INSERT INTO blocks (block_hash, previous_hash, block_data, transaction_count, timestamp, nonce) VALUES (?, ?, ?, ?, ?, ?);";

    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

    if (rc != SQLITE_OK ) {
        fprintf(stderr, "Failed to insert data\n");
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        exit(1);
    }

    sqlite3_bind_text(stmt, 1, new_block.block_hash_hex, strlen(new_block.block_hash_hex), SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, new_block.previous_hash, strlen(new_block.previous_hash), SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, new_block.transaction_bundle, strlen(new_block.transaction_bundle), SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, new_block.transaction_count);

    sqlite3_bind_int(stmt, 5, new_block.timestamp);
    sqlite3_bind_int(stmt, 6, new_block.nonce);

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
