#include "functions.h"

//Run makefile to compile, make

void cleanup_stack(uint8_t *secret_key, size_t secret_key_len) {
	OQS_MEM_cleanse(secret_key, secret_key_len);
}

int create_transaction() {

    struct transaction new_transaction;
    struct enc_wallet wallet_keys;

    sqlite3 *db;
    char *err_msg = 0;
    int rc;
    char sql[99999];
    char tmp_input[128]; //just a temp variable for holding stdin

    rc = sqlite3_open(DB_TRANSACTIONS, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    char *create_table_query = "CREATE TABLE IF NOT EXISTS transactions (txid TEXT PRIMARY KEY, sender TEXT, recipient TEXT, amount REAL, miners_fee REAL, moonshot_fee REAL, data TEXT, data_type INTEGER, timestamp DATETIME, signature TEXT, confirmed INTEGER);";

    rc = sqlite3_exec(db, create_table_query, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }        

    sqlite3_close(db); //close to get address from database

    printf("Enter sender address: "); getchar();
    fgets(new_transaction.sender, sizeof(new_transaction.sender), stdin);

    printf("Enter recipient address: "); getchar();
    fgets(new_transaction.recipient, sizeof(new_transaction.recipient), stdin);

    printf("Enter amount: ");
    fgets(tmp_input, sizeof(tmp_input), stdin);
    tmp_input[strcspn(tmp_input, "\n")] = 0;
    new_transaction.amount = strtold(tmp_input, NULL);
    printf("Amount: %Lf\n", new_transaction.amount);

    printf("Enter transaction fee: ");
    fgets(tmp_input, sizeof(tmp_input), stdin);
    tmp_input[strcspn(tmp_input, "\n")] = 0;
    new_transaction.miners_fee = strtold(tmp_input, NULL);
    printf("Amount: %Lf\n", new_transaction.miners_fee);

    printf("Enter moonshot fee: ");
    fgets(tmp_input, sizeof(tmp_input), stdin);
    tmp_input[strcspn(tmp_input, "\n")] = 0;
    new_transaction.moonshot_fee = strtold(tmp_input, NULL);
    printf("Amount: %Lf\n", new_transaction.moonshot_fee);

    printf("Enter data (smart contract or a text description of the transaction) : "); getchar();
    fgets(new_transaction.data, sizeof(new_transaction.data), stdin);

    /* Getline example
    char *data = NULL;
    size_t data_len = 0;
    printf("Enter data (smart contract or a text description of the transaction) : "); getchar();
    getline(&data, &data_len, stdin);
    data_len = strlen(data);
    if (data[data_len-1] == '\n') {
        data[data_len-1] = '\0';
    }
    */

    printf("What is this data? (1 text, 2 smart contract, 3 image...) : ");
    scanf("%d", &new_transaction.data_type);

    new_transaction.timestamp = time(NULL);

    new_transaction.confirmed = 0; //the higher the number the better

/* Compress non transaction data using huffman encoding

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


char *new_transaction_sender = new_transaction.sender;
char *newline = strchr(new_transaction_sender, '\n');
if (newline) {
    *newline = '\0';
}

/* Check local blockchain for valid transaction */
    /* 
       Do a verification of blockchain.db to determine if transaction is valid 
       Does the sender have the funds
    */

    memset(sql, 0, 99999);

rc = sqlite3_open(DB_WALLET, &db);
if (rc != SQLITE_OK) {
    fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return 1;
}

sqlite3_stmt *stmt;

snprintf(sql, sizeof(sql), "SELECT address, private_key, public_key FROM wallet");

rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
if (rc != SQLITE_OK) {
  fprintf(stderr, "Error preparing SQL statement: %s\n", sqlite3_errmsg(db));
  return 1;
}

int result = sqlite3_step(stmt);
bool address_found = false;
while (result == SQLITE_ROW) {
  const char *address = (const char *)sqlite3_column_text(stmt, 0);
  if (address == NULL) {
    fprintf(stderr, "Error: address is NULL\n");
    return 1;
  }

if (strcmp(address, new_transaction.sender) == 0) {
    address_found = true;

    char *private_key_hex = malloc(OQS_SIG_dilithium_2_length_secret_key_hex);
    snprintf(private_key_hex, OQS_SIG_dilithium_2_length_secret_key_hex, "%s", (const char *)sqlite3_column_text(stmt, 1));
    strncpy(wallet_keys.private_key, private_key_hex, OQS_SIG_dilithium_2_length_secret_key_hex);

    char *public_key_hex = malloc(OQS_SIG_dilithium_2_length_public_key_hex);
    snprintf(public_key_hex, OQS_SIG_dilithium_2_length_public_key_hex, "%s", (const char *)sqlite3_column_text(stmt, 2));
    strncpy(wallet_keys.public_key, public_key_hex, OQS_SIG_dilithium_2_length_public_key_hex);

    break;
  }

  result = sqlite3_step(stmt);
}

if (!address_found) {
    printf("Address does not exist\n");
} else {
    printf("\n\nPublic Key$: %s\n\n", wallet_keys.public_key);
    printf("\n\nPrivate Key$: %s\n\n", wallet_keys.private_key);
}


sqlite3_finalize(stmt);
sqlite3_close(db); //Insert transaction into database and close


    //sign it
    sign_transaction(new_transaction.sender, new_transaction.recipient, new_transaction.amount, wallet_keys.private_key, wallet_keys.public_key, new_transaction.signature);

    printf("\n\nSignature: %s\n\n", new_transaction.signature);

    //Determine the txtid, double sha
    get_txid(&new_transaction);
    printf("Transaction ID: %s\n", new_transaction.txid);

/* Put into the transaction pool, send it, broadcast to the network, they perform verification and block it up... */

    rc = sqlite3_open(DB_TRANSACTIONS, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    int ret = snprintf(sql, sizeof(sql), "INSERT INTO transactions (txid, sender, recipient, amount, miners_fee, moonshot_fee, data, data_type, timestamp, signature, confirmed) VALUES ('%s', '%s', '%s', %Lf, %Lf, %Lf, '%s', '%d', %ld, '%s', '%d');", new_transaction.txid, new_transaction.sender, new_transaction.recipient, new_transaction.amount, new_transaction.miners_fee, new_transaction.moonshot_fee, new_transaction.data, new_transaction.data_type, new_transaction.timestamp,new_transaction.signature, new_transaction.confirmed);

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

void hex_to_bin(const char *hex_str, uint8_t *bin_str, size_t hex_len) {
    for (size_t i = 0; i < hex_len; i += 2) {
        char hex_byte[3] = {hex_str[i], hex_str[i+1], '\0'};
        uint8_t byte = (uint8_t) strtol(hex_byte, NULL, 16);
        bin_str[i/2] = byte;
    }
}

int sign_transaction(uint8_t *sender_address, uint8_t *receiver_address, long double amount, uint8_t *private_key, uint8_t *public_key, uint8_t *signature) {
    // Check input sizes
    if (strlen((const char *) sender_address) == 0 || strlen((const char *) sender_address) > 69) {
        fprintf(stderr, "ERROR: Invalid sender address. Too short.\n");
        return -1;
    }
    if (strlen((const char *) receiver_address) > 69 || strlen((const char *) receiver_address) > 69) {
        fprintf(stderr, "ERROR: Invalid receiver address. Too long.\n");
        return -1;
    }
    if (strlen((const char *) private_key) != OQS_SIG_dilithium_2_length_secret_key_hex - 1) {
        fprintf(stderr, "ERROR: Invalid private key.\n");
        return -1;
    }
    if (strlen((const char *) public_key) != OQS_SIG_dilithium_2_length_public_key_hex - 1) {
        fprintf(stderr, "ERROR: Invalid public key.\n");
        return -1;
    }

    size_t signature_len = OQS_SIG_dilithium_2_length_signature; //sinature always 2420 in Dilithium2, EUF-CMA
    uint8_t tmp_signature[OQS_SIG_dilithium_2_length_signature]; //sinature always 2420 binary and x 2, 4840 hex encoded in Dilithium2

    uint8_t private_key_bin[OQS_SIG_dilithium_2_length_secret_key]; //private_key hex = [5056] bin = [2528]
    hex_to_bin(private_key, private_key_bin, OQS_SIG_dilithium_2_length_secret_key_hex);
   
    // Convert the hex encoded public key to binary
    uint8_t public_key_bin[OQS_SIG_dilithium_2_length_public_key]; //public_key hex = [2624] bin = [1312]
    hex_to_bin(public_key, public_key_bin, OQS_SIG_dilithium_2_length_public_key_hex);
 
    // Hash the message using sha3 384
    EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
    EVP_DigestInit_ex(mdctx, EVP_sha3_384(), NULL);
    EVP_DigestUpdate(mdctx, sender_address, strlen((const char *) sender_address));
    EVP_DigestUpdate(mdctx, ":", 1);
    EVP_DigestUpdate(mdctx, receiver_address, strlen((const char *) receiver_address));
    EVP_DigestUpdate(mdctx, ":", 1);
    char amount_str[20];
    snprintf(amount_str, sizeof(amount_str), "%.8Lf", amount);
    EVP_DigestUpdate(mdctx, amount_str, strlen(amount_str));
    uint8_t message_digest[EVP_MAX_MD_SIZE];
    unsigned int message_digest_len;
    EVP_DigestFinal_ex(mdctx, message_digest, &message_digest_len);
    EVP_MD_CTX_destroy(mdctx);

    OQS_STATUS rc;

/*  Debug
    uint8_t message_digest[62];
    unsigned int message_digest_len = 62;
    // let's create a random test message to sign
    OQS_randombytes(message_digest, message_digest_len);
*/

    // Sign the message using dlithium2 
    rc = OQS_SIG_dilithium_2_sign(tmp_signature, &signature_len, message_digest, message_digest_len, private_key_bin);
    if (rc != OQS_SUCCESS) {
		fprintf(stderr, "ERROR: OQS_SIG_dilithium_2_sign failed!\n");
        fprintf(stderr, "ERROR: OQS function failed with error code %d.\n", rc);
		cleanup_stack(private_key, OQS_SIG_dilithium_2_length_secret_key);
		return OQS_ERROR;
	}
	
    rc = OQS_SIG_dilithium_2_verify(message_digest, message_digest_len, tmp_signature, signature_len, public_key_bin);
	if (rc != OQS_SUCCESS) {
		fprintf(stderr, "ERROR: OQS_SIG_dilithium_2_verify failed!\n");
        fprintf(stderr, "ERROR: OQS function failed with error code %d.\n", rc);
		cleanup_stack(private_key, OQS_SIG_dilithium_2_length_secret_key);
		return OQS_ERROR;
	}

    for (int i = 0; i < OQS_SIG_dilithium_2_length_signature; i++)
        sprintf(signature + (i * 2), "%02x",  tmp_signature[i]);

    printf("Signature OK!");
}

void get_txid(struct transaction *tx) {
    unsigned char hash[SHA384_DIGEST_LENGTH];
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();

    EVP_DigestInit(ctx, EVP_sha3_384());
    EVP_DigestUpdate(ctx, (unsigned char *)tx, sizeof(struct transaction));
    EVP_DigestFinal(ctx, hash, NULL);

    EVP_MD_CTX_free(ctx);

    int i;
    for (i = 0; i < SHA384_DIGEST_LENGTH; i++)
        sprintf(tx->txid + (i * 2), "%02x", hash[i]);
}
