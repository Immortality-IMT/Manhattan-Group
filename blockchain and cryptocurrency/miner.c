#include "functions.h"

/*
   Grab transactions from the transaction pool (transactions.db) and make a block
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
        //printf("%s", output);
    }

    sqlite3_free_table(result);
    sqlite3_close(db);

    struct block new_block;
    new_block.timestamp = time(NULL);
    strcpy(new_block.transaction_bundle, output);
    strcpy(new_block.previous_hash, get_previous_hash());
    strcpy(new_block.block_hash, get_block_hash(new_block)); //todo: returning random string atm

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


/* alphabet: [a-z0-9] */
const char alphabet[] = "abcdefghijklmnopqrstuvwxyz0123456789";
int intN(int n) { return rand() % n; }

char *randomString(int len) {
  char *rstr = malloc((len + 1) * sizeof(char));
  int i;
  for (i = 0; i < len; i++) {
    rstr[i] = alphabet[intN(strlen(alphabet))];
  }
  rstr[len] = '\0';
  return rstr;
}

char* get_block_hash(struct block new_block) {
/*
    unsigned char hash[SHA256_DIGEST_LENGTH];
    char input[sizeof(new_block.timestamp) + sizeof(new_block.transaction_bundle) + sizeof(new_block.previous_hash)];
    sprintf(input, "%d%s%s", new_block.timestamp, new_block.transaction_bundle, new_block.previous_hash);

    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input, strlen(input));
    SHA256_Final(hash, &sha256);

    *output = (char*)malloc(sizeof(hash) + 1);
    int i;
    for(i = 0; i < SHA256_DIGEST_LENGTH; i++)
        sprintf(*output + (i * 2), "%02x", hash[i]);
    (*output)[SHA256_DIGEST_LENGTH*2] = 0;
*/

    char* str = randomString(20);
    
    return str;
}

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

    char *sql = "INSERT INTO blocks (block_hash, previous_hash, data, timestamp) VALUES (?, ?, ?, ?);";

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
   
    struct block new_block = create_block();
    add_block(new_block);
    printf("Block added to blockchain: %s\n", new_block.block_hash);
    return 0;

}
