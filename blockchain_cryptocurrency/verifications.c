#include "functions.h"

/* TODO the more verifications the better over everything */

//Verifications, there are several ways of performing verifications
//These are just placeholders and not used yet.

//Transactions
//Basic validate the transaction by checking the signature
int validate_transaction(struct transaction *t) {
    // Concatenate the data to be hashed
    char data[9999];
    sprintf(data, "%s%s%f", t->sender, t->recipient, t->amount); //re-hash the same data

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
