#include "functions.h"

/* 
- Wallet, generate keys, encode keys, store them in wallet.db
- Transactions, sender key, receiver key, amount, add to temp transactions.db
- Mine, transactions into blocks, add to blockchain.db
- Add block to blockchain, add to blockchain.db
- Various shas, verifications, proofs, preferences, standardized accorss the network

Compile with...
    gcc -g -o wallet transactions.c wallet.c verifications.c functions.h miner.c blockchain.c base58.c keccak.c -lssl -lcrypto -lsqlite3 -lz -loqs 
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
*/

//Generate a private and public key pair, using CRYSTALS-Dilithium
void generate_key_pair(uint8_t *public_key, uint8_t *private_key, size_t public_key_length, size_t private_key_length) {

    //private key is always 2528 bytes;
    //public key is always 1312 bytes;
    //private and public key are encoded to hex to differentiate it from the address base 58
    //Generate public key, using private key

    memset(public_key, 0, public_key_length);
    memset(private_key, 0, private_key_length);


    OQS_SIG *sig = OQS_SIG_new(OQS_SIG_alg_dilithium_2);
    if (sig == NULL) {
        fprintf(stderr, "Failed to create a new signature object.\n");
        return;
    }

    if (public_key == NULL) {
        OQS_SIG_free(sig);
        fprintf(stderr, "Failed to allocate memory for public key.\n");
        return;
    }

    int ret = OQS_SIG_keypair(sig, public_key, private_key);
    if (ret != OQS_SUCCESS) {
        OQS_SIG_free(sig);
        fprintf(stderr, "Failed to generate the keypair.\n");
        return;
    }

    OQS_SIG_free(sig);
}

//address is the hash of the public key using sha3_384 bit
void generate_new_address(uint8_t *public_key, uint8_t *address) {

    memset(address, 0, 48);

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha3_384(), NULL);
    EVP_DigestUpdate(ctx, public_key, 1312);
    EVP_DigestFinal_ex(ctx, address, NULL);
    EVP_MD_CTX_free(ctx);

}

void public_key_to_hex(unsigned char* public_key, char hex_public_key[66]) {
  int i;
  for (i = 0; i < 32; i++) {
    sprintf(hex_public_key + (i * 2), "%02X", public_key[i]);
  }
}

int generate_address() {

    sqlite3 *db;
    int rc;
    char *err_msg = 0;

    char sql[20000]; //statement to copy keys statement to sqlite database wallet.db
    struct enc_wallet h_wallet; //takes the converted copy from bin to hex
    struct bin_wallet b_wallet; //used until final conversion copy from bin to hex

    /*
     new_wallet.public_key; 1312 //use address instead
     new_wallet.private_key; 2528 //secret key used to secure funds
     new_wallet.address; raw 48 base58 approx 100 //what you send to get pay or to pay
     new_wallet.statement; //bank statement of past transactions
     new_wallet.balance; //how much money you have
    */

    rc = sqlite3_open(DB_WALLET, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    const char * create_wallet_table = "CREATE TABLE IF NOT EXISTS wallet (address TEXT PRIMARY KEY, private_key TEXT, public_key TEXT, statement TEXT, balance TEXT);";

    rc = sqlite3_exec(db, create_wallet_table, 0, 0, &err_msg);

    if (rc != SQLITE_OK ) {
        fprintf(stderr, "Failed to create table: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }


        for (int i = 0; i < 5; i++) {

            memset(h_wallet.public_key, 0, sizeof(h_wallet.public_key));
            memset(h_wallet.private_key, 0, sizeof(h_wallet.private_key));

            //generate both private and public key pair
            generate_key_pair(b_wallet.public_key, b_wallet.private_key, sizeof(b_wallet.public_key), sizeof(b_wallet.private_key));

            //private and public key stored as hex, address as base58
            generate_new_address(b_wallet.public_key, b_wallet.address); //generate address from public key

            for (int x = 0; x < 1312; x++) {
              int n = snprintf(h_wallet.public_key + (x * 2), 3, "%02X", b_wallet.public_key[x]);
              if (n < 0) {
                // handle error
              }
            }

            printf("\nPublic Key: %s\n\n", h_wallet.public_key);

            for (int x = 0; x < 2528; x++) {
              int n = snprintf(h_wallet.private_key + (x * 2), 3, "%02X", b_wallet.private_key[x]);
              if (n < 0) {
                // handle error
              }
            };

            printf("\nPrivate Key: %s\n\n", h_wallet.private_key);

            /* Hex encoded
            for (int x = 0; x < 48; x++) {
              int n = snprintf(h_wallet.address + (x * 2), 3, "%02X", b_wallet.address[x]);
              if (n < 0) {
                // handle error
              }
            };
            */

            //Encode the address to base58
            size_t base58_address_len = 96; //sha3 384 encodes to about 64, so 96 to be sure
            //memset(b_wallet.address, 0, sizeof(b_wallet.address)); 

            b58enc(h_wallet.address, &base58_address_len, b_wallet.address, 48);
            printf("\nBase58: %s\n", h_wallet.address);

            //Insert the address into the database
  int ret = snprintf(sql, sizeof(sql), "INSERT INTO wallet (address, private_key, public_key, balance) VALUES ('%s','%s','%s','%s');", h_wallet.address, h_wallet.private_key, h_wallet.public_key, "0");
            rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
            if (rc != SQLITE_OK ) {
                fprintf(stderr, "Failed to insert address: %s\n", err_msg);
                sqlite3_free(err_msg);
            }
        }

    printf("\nSuccessfully stored addresses in the wallet.\n");

    sqlite3_close(db);

}

void print_table(const char* db_name, const char* table_name) {
    sqlite3 *db;
    char *err_msg = 0;
    int rc = sqlite3_open(db_name, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    char sql[4096];
    sprintf(sql, "SELECT * FROM %s", table_name);

    rc = sqlite3_exec(db, sql, callback, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }

    sqlite3_close(db);
}

/*
static int callback(void *data, int argc, char **argv, char **col_name) {
    for (int i = 0; i < argc; i++) {
        printf("%s = %s\n", col_name[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}*/

static int callback(void *data, int argc, char **argv, char **col_name) {

/* To use huffman coding compress, to decompress before echo out
int decompressed_data_len = 0;

    for (int i = 0; i < argc; i++) {
        if (strcmp(col_name[i], "data") == 0) {

            char *decompressed_data = (char *) malloc(CHUNK);
            memset(decompressed_data, 0, CHUNK); //clear the mem so no jink chars are present

            int decompressed_data_len = 0;
            int ret = decompress_data(argv[i], strlen(argv[i]), &decompressed_data, &decompressed_data_len);
            if (ret != 0) {
                printf("Failed to decompress data\n");
                return 1;
            }

            printf("%s = %s\n", col_name[i], decompressed_data);
            free(decompressed_data);

        } else {
            printf("%s = %s\n", col_name[i], argv[i] ? argv[i] : "NULL");
        }
    }
*/

    for (int i = 0; i < argc; i++) {
        printf("%s = %s\n", col_name[i], argv[i] ? argv[i] : "NULL");
    }

    printf("\n");
    return 0;
}

int main() {

	int choice = 0, n = 0, whichIndex = 0;
	char sender, receiver;
	int inputnums = 0;
	float money = 0;

struct transaction t;
struct block b;

    while(1) {

    	printf("\nBlockchain demonstrator application <transaction account>\t\n");
    	printf("Please enter a number on your keypad\n");
    	printf("\n(1) Generate new addresses\n(2) Print all addresses\n(3) Make a transaction\n(4) Print out transaction pool\n(5) Mine transactions into a block\n(6) Print out blockchain\n(7) Verify transaction\n(8) Verify block\n(9) Verify blockchain\n(10) Exit\n");
        printf("Enter a choice: ");

    	scanf("%d",&choice);
    
    		switch(choice){

    			case 1:
    				printf("\nWallet: Generating 5 new address into wallets.db\n\n");
                    generate_address();
    				break;
    			case 2:
    				printf("\nWallet: Printing out wallet database (wallet.db)\n\n");
                    print_table(DB_WALLET, "wallet");
    				break;
                case 3:
    				printf("Transaction: Make a new transaction, enter address of both parties and the amount of money\n");
                    printf("e.g. A6i+1ZCmn0hTc5rwXTZtO3V7GqmQBnz8OA65JZ+zxoQ=, AzpGlJ0vRgNVpHc2vSJ14pWbnn02mc7Az3C88y74GTM=, 50)\n");
    				create_transaction();
    				break;
    			case 4:
    				printf("\nTransaction: Printing out transaction pool (transactions.db)\n\n");
                    print_table(DB_TRANSACTIONS, "transactions");
    				break;
    			case 5:
                    printf("Mining: Generate block (create_block)\n\n");
    				mine(); //TODO allow the miner to choose which transactions to mine 
       				break;
    			case 6:
    				printf("Print out blockchain:\n\n");
                    start_blockchain();
                    print_table(DB_BLOCKCHAIN, "blocks");
                	break;
    			case 7:
    				printf("Blockchain: Verify transaction:\n\n");//
                        int result = validate_transaction(&t);
                            if (result) {
                            printf("Transaction is valid\n");
                            } else {
                            printf("Transaction is invalid\n");
                            }
    				break;
    			case 8:
    				printf("Blockchain: Verify block:\n\n");//
    				    validate_block(&b, 3);

                         result = validate_block(&b, 3);
                            if (result) {
                            printf("Block is valid\n");
                            } else {
                            printf("Block is invalid\n");
                            }
    				break;
    			case 9:
    				printf("Blockchain: Verify blockchain:\n\n");//
                         result = validate_blockchain(&b, 100);
                            if (result) {
                            printf("Block is valid\n");
                            } else {
                            printf("Block is invalid\n");
                            }
    				break;
                case 10:
                    return 0;
                    break;
    			default:
    				printf("Only numbers between 1 and 10\n");
    				break;
    		    }
    	}

    return 0;
}

