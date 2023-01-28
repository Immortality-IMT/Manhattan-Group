#include "functions.h"

//gcc -g -o wallet transactions.c wallet.c verifications.c functions.h miner.c blockchain.c -lssl -lcrypto -lsqlite3 -lz

static int callback_count(void *transactions, int argc, char **argv, char **azColName) {
    int *count = (int*)transactions;
    *count = atoi(argv[0]);
    return 0;
}

int start_blockchain() {

//get the blockchain off the internet, off another node and store it locally

struct block genesis_block;

    strcpy(genesis_block.previous_hash, "EPOCH");
    strcpy(genesis_block.block_hash, "EPOCH");
    strcpy(genesis_block.transaction_bundle, "EPOCH");
    genesis_block.timestamp = time(NULL);
 
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    char *sql;

    // Open or create a new database
    rc = sqlite3_open(DB_BLOCKCHAIN, &db);
    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return(1);
    }

    // Create a table to store the blocks
    sql = "CREATE TABLE IF NOT EXISTS blocks(block_hash TEXT PRIMARY KEY, previous_hash TEXT, block_data TEXT, timestamp INTEGER);";
    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
    if( rc != SQLITE_OK ) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    int row_count = 0;

    // Check if the table is empty
    rc = sqlite3_exec(db, "SELECT COUNT(*) FROM blocks", callback_count, &row_count, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

   if (row_count == 0) {
        // Insert the genesis block into the table
        time_t now = time(NULL);
        genesis_block.timestamp = now - 1;

        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(db, "INSERT INTO blocks (block_hash, previous_hash, block_data, timestamp) VALUES (?,?,?,?)", -1, &stmt, NULL);

        sqlite3_bind_text(stmt, 1, genesis_block.block_hash, strlen(genesis_block.block_hash), SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, genesis_block.previous_hash, strlen(genesis_block.previous_hash), SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, genesis_block.transaction_bundle, strlen(genesis_block.transaction_bundle), SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 4, genesis_block.timestamp);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

        // Close the database connection
        sqlite3_close(db);
        return 0;
}
