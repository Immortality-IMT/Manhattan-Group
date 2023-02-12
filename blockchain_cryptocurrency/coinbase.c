#include "functions.h"

struct CoinbaseTransaction {
  char prev_block_hash[32];
  char coinbase_output[8];
  int nonce;
};

void create_coinbase_transaction(struct CoinbaseTransaction *coinbase, char prev_block_hash[32], int nonce) {
  memcpy(coinbase->prev_block_hash, prev_block_hash, 32);
  memset(coinbase->coinbase_output, 0, 8);
  coinbase->nonce = nonce;
}

void print_coinbase_transaction(struct CoinbaseTransaction *coinbase) {
  printf("Previous Block Hash: ");
  for (int i = 0; i < 32; i++) {
    printf("%02x", coinbase->prev_block_hash[i]);
  }
  printf("\n");

  printf("Coinbase Output: %d\n", *((int*)coinbase->coinbase_output));

  printf("Nonce: %d\n", coinbase->nonce);
}

int main() {
  struct CoinbaseTransaction coinbase;
  char prev_block_hash[32] = {0};
  int nonce = 12345;

  create_coinbase_transaction(&coinbase, prev_block_hash, nonce);
  print_coinbase_transaction(&coinbase);

  return 0;
}

