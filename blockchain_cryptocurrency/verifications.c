#include "functions.h"

/* TODO the more verifications the better over everything */


//are newly genererated key usable and valid
int validate_key_pair(uint8_t *public_key, uint8_t *private_key, size_t public_key_length, size_t private_key_length) {
    if (public_key == NULL || private_key == NULL) {
        fprintf(stderr, "Failed to allocate memory for public key.\n");
        return 1;
    }

    OQS_SIG *sig = OQS_SIG_new(OQS_SIG_alg_dilithium_2);
    if (sig == NULL) {
        fprintf(stderr, "Failed to create a new signature object.\n");
        return 2;
    }

	size_t message_len = 50;
    uint8_t message[message_len];

	size_t signature_len;

    uint8_t signature[OQS_SIG_dilithium_2_length_signature];
    memset(signature, 0, OQS_SIG_dilithium_2_length_signature);

    OQS_randombytes(message, message_len);

    int rc = OQS_SIG_dilithium_2_sign(signature, &signature_len, message, message_len, private_key);

    if (rc != OQS_SUCCESS) {
		fprintf(stderr, "ERROR: OQS_SIG_dilithium_2_sign failed!\n");
		cleanup_stack(private_key, OQS_SIG_dilithium_2_length_secret_key);
		return 1; //OQS_ERROR;
	}

    rc = OQS_SIG_dilithium_2_verify(message, message_len, signature, signature_len, public_key);
	if (rc != OQS_SUCCESS) {
		fprintf(stderr, "ERROR: OQS_SIG_dilithium_2_verify failed!\n");
		cleanup_stack(private_key, OQS_SIG_dilithium_2_length_secret_key);
		return 2; //OQS_ERROR;
	}

    printf("Keypair passed! - OQS_SIG_dilithium_2 operations completed.\n");

    OQS_SIG_free(sig);
    return 0;
}

int hex2bin(unsigned char *p, const char *hexstring, size_t len) {
    size_t i;

    for (i = 0; i < len; i++) {
        unsigned int x;
        int ret = sscanf(hexstring + (i * 2), "%2x", &x);
        if (ret != 1) {
            return -1;
        }
        p[i] = x;
    }
    return 0;
}


int test_keys_in_wallet()
{
    sqlite3 *db;
    int rc;
    char *sql;
    sqlite3_stmt *res;
    const char *tail;
    int public_key_length, private_key_length;
    char *public_key_hex, *private_key_hex;

    rc = sqlite3_open("DB/wallet.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    sql = "SELECT public_key, private_key FROM wallet";
    rc = sqlite3_prepare_v2(db, sql, strlen(sql), &res, &tail);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to retrieve data: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 2;
    }

    while (sqlite3_step(res) == SQLITE_ROW) {
        public_key_hex = (char *)sqlite3_column_text(res, 0);
        private_key_hex = (char *)sqlite3_column_text(res, 1);

        public_key_length = strlen(public_key_hex) / 2;
        uint8_t public_key[public_key_length];

        private_key_length = strlen(private_key_hex) / 2;
        uint8_t private_key[private_key_length];

        hex2bin(public_key, public_key_hex, public_key_length);
        hex2bin(private_key, private_key_hex, private_key_length);

        int result = validate_key_pair(public_key, private_key, public_key_length, private_key_length);
        if (result == 0) {
            printf("Keypair passed!\n");
        } else {
            printf("Keypair failed!\n");
        }
    }

    sqlite3_finalize(res);
    sqlite3_close(db);

    return 0;
}


//Verifications, there are several ways of performing verifications
//These are just placeholders and not used yet.

//Transactions
//Basic validate the transaction by checking the signature
int validate_transaction(struct transaction *t) {
    // Concatenate the data to be hashed
    char data[9999];
    sprintf(data, "%s%s%Lf", t->sender, t->recipient, t->amount); //re-hash the same data

    // Hash the data using SHA384
    unsigned char hash[SHA384_DIGEST_LENGTH];
    SHA384((unsigned char *)data, strlen(data), hash);

    // Compare the hash with the transaction signature
    if (memcmp(hash, t->signature, SHA384_DIGEST_LENGTH) == 0) {
        // Signature is valid, return 1
        return 1;
    } else {
        // Signature is invalid, return 0
        return 0;
    }

/**************************************************************************
 Does the user have enough funds, query a local and  foreign blockchain
***************************************************************************/

/*
 (1)
  the sending address must be on the blockchain
  and with the funds suggested calculated applicable to the sender.
  this is checked by the foreign blockchain
  which the sender and the reciever have no
  ability to alter.

  A number of blockchains each return to provide the verification.

 (2)
  the sender signs the transaction with encryption, something only the 
  owner of the address can do.
*/

    // Validate the transaction
/*
    int result = validate_transaction(&t);
    if (result) {
        printf("Transaction is valid\n");
    } else {
        printf("Transaction is invalid\n");
    }
*/
}

//Block
int validate_block(struct block *b, int difficulty) {
    // Calculate the block hash using the same method as when the block was first hashed
    char data[29999];
    sprintf(data, "%d%s%ld%d%s%d", b->block_index, b->previous_hash, b->timestamp, b->transaction_count, b->transaction_bundle, b->nonce);

    unsigned char calculated_hash[SHA384_DIGEST_LENGTH];
    SHA384((unsigned char *)data, strlen(data), calculated_hash);

    // Check if the calculated hash has the required number of leading zeros
    int leading_zeros = 0;
    for (int i = 0; i < difficulty; i++) {
        if (calculated_hash[i] == 0) {
            leading_zeros++;
        } else {
            break;
        }
    }

    if (leading_zeros == difficulty) {
        return 1;
    } else {
        return 0;
    }
}

// Basic function to verify a blockchain
int validate_blockchain(struct block *blocks, int num_blocks) {
    for (int i = 1; i < num_blocks; i++) {
        if (strcmp(blocks[i].previous_hash, blocks[i - 1].block_hash) != 0) {
            return 0;
        }
    }
    return 1;
}
