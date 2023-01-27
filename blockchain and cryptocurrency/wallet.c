#include "functions.h"

/* 
   1) Wallet -> Generate keys
   2) Transactions -> sender key, receiver key, amount, add to trans.db
   3) Mine -> transactions into blocks, add to block.db
   4) Add block to blockchain, add to blockchain.db
   5) Shas, Verifications, Proofs, Preferences, standardized accorss the network

gcc -g -o wallet transactions.c wallet.c verifications.c functions.h miner.c blockchain.c -lssl -lcrypto -lsqlite3 -lz
*/

//gcc -g -o wallet transactions.c wallet.c functions.h -lssl -lcrypto -lsqlite3

//Generate private key
BIGNUM* private_key() {
    BIGNUM* private_key = BN_new();
    if (!BN_rand(private_key, 256, -1, 0)) {
        // handle error
    }
    return private_key;
}

//Generate assoc public key
unsigned char* public_key(BIGNUM* private_key) {
    EC_GROUP* ecgroup = EC_GROUP_new_by_curve_name(NID_secp256k1);
    EC_POINT* pub_key = EC_POINT_new(ecgroup);
    EC_POINT_mul(ecgroup, pub_key, private_key, NULL, NULL, NULL);

    unsigned char* pub_key_ptr;
    size_t len = EC_POINT_point2oct(ecgroup, pub_key, POINT_CONVERSION_COMPRESSED, NULL, 0, NULL);
    pub_key_ptr = (unsigned char*)malloc(len);
    EC_POINT_point2oct(ecgroup, pub_key, POINT_CONVERSION_COMPRESSED, pub_key_ptr, len, NULL);

    EC_GROUP_free(ecgroup);
    EC_POINT_free(pub_key);
    return pub_key_ptr;
}

char* public_key_to_hex(unsigned char* public_key) {
    int i;
    char *hex_str = (char *)malloc(65);
    for (i = 0; i < 32; i++) {
        sprintf(hex_str + (i * 2), "%02x", public_key[i]);
    }
    hex_str[64] = '\0';
    return hex_str;
}

//Base64
char* public_key_to_address(unsigned char* public_key) {
    // perform base64 encoding on the public key
    BIO *bmem = BIO_new(BIO_s_mem());
    BIO *b64 = BIO_new(BIO_f_base64());
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, public_key, 32);
    BIO_flush(b64);
    BUF_MEM *bptr;
    BIO_get_mem_ptr(b64, &bptr);
    char *address = (char *)malloc(bptr->length);
    memcpy(address, bptr->data, bptr->length-1);
    address[bptr->length-1] = 0;
    BIO_free_all(b64);

    return address;
}


//Encode the keys, private to base64, public to hex, public to address
void get_keys(BIGNUM* private_key_ptr,unsigned char* public_key_ptr, char* private_key_b64, char* public_key_b64, char* address) {
    int private_key_len = BN_num_bytes(private_key_ptr);
    unsigned char* private_key_data = (unsigned char*)malloc(private_key_len);
    BN_bn2bin(private_key_ptr, private_key_data);
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, private_key_data, private_key_len);
    BIO_flush(b64);
    BUF_MEM *bptr;
    BIO_get_mem_ptr(b64, &bptr);
    memcpy(private_key_b64, bptr->data, bptr->length);
    private_key_b64[bptr->length-1] = '\0';
    BIO_free_all(b64);
    free(private_key_data);
    // convert public key to hex
    strcpy(public_key_b64, public_key_to_hex(public_key_ptr));
    // convert public key to address
    strcpy(address, public_key_to_address(public_key_ptr));
}

int generate_address() {

    sqlite3 *db;
    int rc;
    char *err_msg = 0;

    struct wallet new_wallet;
    /*
        new_wallet.public_key_b64; //use address instead
        new_wallet.private_key_b64; //secret key used to ensure your identity
        new_wallet.address; //what you send to get paid or to pay
        new_wallet.statement; //bank statement of past transactions
        new_wallet.balance; //how much money you have
    */

    rc = sqlite3_open(DB_WALLET, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    const char * create_wallet_table = "CREATE TABLE IF NOT EXISTS wallet (address TEXT PRIMARY KEY, private_key_b64 TEXT, public_key_b64 TEXT, statement TEXT, balance TEXT);";

//    const char *create_wallet_table = "CREATE TABLE IF NOT EXISTS wallet (address TEXT PRIMARY KEY, private_key_b64 TEXT, public_key_b64 TEXT);";

    rc = sqlite3_exec(db, create_wallet_table, 0, 0, &err_msg);

    if (rc != SQLITE_OK ) {
        fprintf(stderr, "Failed to create table: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    

        char private_key_b64[256];
        char public_key_b64[256];
        char address[256];
        char sql[1024];

            for (int i = 0; i < 5; i++) {

                BIGNUM* private_key_ptr = private_key();
                unsigned char* public_key_ptr = public_key(private_key_ptr);

/*                get_keys(private_key_ptr, public_key_ptr, private_key_b64, public_key_b64, address);

                printf("Private Key (base64): %s\n", private_key_b64);
                printf("Public Key (hex): %s\n", public_key_b64);
                printf("Address: %s\n", address);
*/
                get_keys(private_key_ptr, public_key_ptr, new_wallet.private_key_b64, new_wallet.public_key_b64, new_wallet.address);

                printf("Private Key (base64): %s\n", new_wallet.private_key_b64);
                printf("Public Key (hex): %s\n", new_wallet.public_key_b64);
                printf("Address: %s\n", new_wallet.address);

                // insert the address into the database
                snprintf(sql, sizeof(sql), "INSERT INTO wallet (address, private_key_b64, public_key_b64, balance) VALUES ('%s','%s','%s','%s');", new_wallet.address, new_wallet.private_key_b64, new_wallet.public_key_b64, "0");
//                snprintf(sql, sizeof(sql), "INSERT INTO wallet (address, private_key_b64, public_key_b64) VALUES ('%s','%s','%s');", address, private_key_b64, public_key_b64);
                rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
                if (rc != SQLITE_OK ) {
                    fprintf(stderr, "Failed to insert address: %s\n", err_msg);
                    sqlite3_free(err_msg);
                }

            }

    printf("Successfully stored addresses in the wallet.\n");

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

    char sql[256];
    sprintf(sql, "SELECT * FROM %s", table_name);

    rc = sqlite3_exec(db, sql, callback, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }

    sqlite3_close(db);
}

static int callback(void *data, int argc, char **argv, char **col_name) {
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
    				verify_transaction();
    				break;
    			case 8:
    				printf("Blockchain: Verify block:\n\n");//
    				verify_block();
    				break;
    			case 9:
    				printf("Blockchain: Verify blockchain:\n\n");//
    				verify_chain();
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

