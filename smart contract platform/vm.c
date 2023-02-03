/*
C code defines a virtual machine to execute opcodes, specifically for smart contract execution. The opcodes that are executed by the VM are encoded in bytecode, which is a low-level, binary representation of the instructions that the VM should perform. The "execute_contract" function in this code takes in bytecode as input and uses a loop and a switch statement to execute the corresponding opcode. The struct "evm_state" contains several arrays for storing data, such as the stack and memory, as well as several integer variables for storing information such as the stack pointer and balance. t doesn't handle gas, it doesn't provide a way to access the blockchain, and it doesn't handle the interaction between smart contracts

//gcc -g -o vm vm.c -lm

//require evm compatible bytecode to execute as smart_contract.bin in the currency directory

https://ethereum.org/en/developers/docs/evm/opcodes/
https://www.evm.codes/?fork=merge
http://ref.x86asm.net/coder32.html#x00
https://github.com/crytic/evm-opcodes
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#define MAX_STACK_SIZE 1024
#define MAX_MEMORY_SIZE 256000

typedef struct {
  int stack[MAX_STACK_SIZE];
  int stack_pointer;
  int memory[MAX_MEMORY_SIZE];
  int memory_size;
  int storage[MAX_MEMORY_SIZE];
  int balance;

  int address;
  int origin;
  int caller;
  int * calldata;
  int calldata_size;
  int code_size;
  int * code;
  int gas;
  int gas_price;
  int gas_limit;
  int coinbase;
  int timestamp;
  int block_number;
  int difficulty;
  int value;

}
evm_state;

const char * load_contract_from_file(const char * file_path) {
  // Open the file

  FILE * fp = fopen(file_path, "r");
  if (!fp) {
    printf("Error: unable to open file %s\n", file_path);
    return NULL;
  }

  // Determine the size of the file
  fseek(fp, 0, SEEK_END);
  size_t file_size = ftell(fp);
  rewind(fp);

  // Allocate memory for the bytecode
  char * bytecode = (char * ) malloc(file_size + 1);
  if (!bytecode) {
    printf("Error: unable to allocate memory for bytecode\n");
    fclose(fp);
    return NULL;
  }

  // Read the bytecode from the file
  size_t bytes_read = fread(bytecode, sizeof(char), file_size, fp);
  if (bytes_read != file_size) {
    printf("Error: unexpected number of bytes read\n");
    free(bytecode);
    fclose(fp);
    return NULL;
  }

  // Null-terminate the bytecode
  bytecode[file_size] = '\0';

  // Close the file
  fclose(fp);

  // Return the bytecode
  return bytecode;
}

void load_smart_contract(evm_state * state,
  const char * bytecode, int bytecode_size) {
  int i;
  for (i = 0; i < bytecode_size; i++) {
    state -> memory[i] = bytecode[i];
  }
  //memcpy(state->memory, bytecode, bytecode_size);
  state -> memory_size = bytecode_size;
}

void load_contract_to_memory(evm_state * state,
  const char * bytecode) {
  int bytecode_length = strlen(bytecode);
  if (bytecode_length >= MAX_MEMORY_SIZE) {
    printf("Exeeds MAX_MEMEORY");
    // Handle error: bytecode is too large to fit in memory
    return;
  }

  int i;
  for (i = 0; i < bytecode_length; i++) {
    state -> memory[i] = bytecode[i];
  }
  state -> memory_size = i;
}

//execute_contract(): A function that takes a smart contract bytecode and a set of input parameters, and executes the contract's code.
void execute_contract(evm_state * state, const char * bytecode) {
  int opcode;
  int a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p;
  int pc = 0;
  while (bytecode[pc] != 0x00) { //	STOP 	Halts execution
    opcode = bytecode[pc++];
    printf("Current opcode: %d\n", opcode);
    switch (opcode) {
    case 0x01: // ADD 	Addition operation
    b = state->stack[--state->stack_pointer];
    a = state->stack[--state->stack_pointer];
    if (state->stack_pointer >= 0 && state->stack_pointer < MAX_STACK_SIZE) {
        state->stack[state->stack_pointer++] = a + b;
        printf("a: %d, b: %d, stack: %d\n", a, b, state->stack[state->stack_pointer - 1]);
    }
    break;
    case 0x02: // MUL 	Multiplication operation
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      state -> stack[state -> stack_pointer++] = a * b;
      break;
    case 0x03: // SUB 	Subtraction operation
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      state -> stack[state -> stack_pointer++] = a - b;
      break;
    case 0x04: // DIV 	Integer division operation
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      state -> stack[state -> stack_pointer++] = a / b;
      break;
    case 0x05: // SDIV 	Signed integer division operation (truncated)
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      state -> stack[state -> stack_pointer++] = ((a ^ INT_MIN) / (b ^ INT_MIN)) ^ INT_MIN;
      break;
    case 0x06: // MOD 	Modulo remainder operation
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      state -> stack[state -> stack_pointer++] = a % b;
      break;
    case 0x07: // SMOD 	Signed modulo remainder operation
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      state -> stack[state -> stack_pointer++] = ((a % b) + b) % b;
      break;
    case 0x08: // ADDMOD 	Modulo addition operation
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      c = state -> stack[--state -> stack_pointer];
      state -> stack[state -> stack_pointer++] = (a + b) % c;
      break;
    case 0x09: // MULMOD 	Modulo multiplication operation
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      c = state -> stack[--state -> stack_pointer];
      state -> stack[state -> stack_pointer++] = (a * b) % c;
      break;
    case 0x0A: // EXP 	Exponential operation
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      state -> stack[state -> stack_pointer++] = pow(a, b);
      break;
    case 0x0B: // SIGNEXTEND 	Extend length of two's complement signed integer
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      if ((a < 32) && (b & (1 << (a - 1)))) {
        state -> stack[state -> stack_pointer++] = b | (0xffffffff << a);
      } else {
        state -> stack[state -> stack_pointer++] = b;
      }
      break;
      //0C-0F	unsued / invalid
    case 0x10: // LT 	Less-than comparison
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      state -> stack[state -> stack_pointer++] = (a < b) ? 1 : 0;
      break;
    case 0x11: // GT 	Greater-than comparison
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      state -> stack[state -> stack_pointer++] = (a > b) ? 1 : 0;
      break;
    case 0x12: // SLT 	Signed less-than comparison
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      state -> stack[state -> stack_pointer++] = (a < b) ? 1 : 0;
      break;
    case 0x13: // SGT 	Signed greater-than comparison
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      state -> stack[state -> stack_pointer++] = (a > b) ? 1 : 0;
      break;
    case 0x14: // EQ 	Equality comparison
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      state -> stack[state -> stack_pointer++] = (a == b) ? 1 : 0;
      break;
    case 0x15: // ISZERO 	Simple not operator
      a = state -> stack[--state -> stack_pointer];
      state -> stack[state -> stack_pointer++] = (a == 0) ? 1 : 0;
      break;
    case 0x16: // AND 	Bitwise AND operation
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      state -> stack[state -> stack_pointer++] = a & b;
      break;
    case 0x17: // OR 	Bitwise OR operation
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      state -> stack[state -> stack_pointer++] = a | b;
      break;
    case 0x18: // XOR 	Bitwise XOR operation
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      state -> stack[state -> stack_pointer++] = a ^ b;
      break;
    case 0x19: // NOT 	Bitwise NOT operation
      a = state -> stack[--state -> stack_pointer];
      state -> stack[state -> stack_pointer++] = ~a;
      break;
    case 0x1A: // BYTE 	Retrieve single byte from word
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      state -> stack[state -> stack_pointer++] = (b >> (8 * a)) & 0xFF;
      break;
       // 0x1b 	SHL 	    Shift Left
       // 0x1c 	SHR 	    Logical Shift Right
       // 0x1d 	SAR 	    Arithmetic Shift Right
       // 1E-1F	invalid
       // 0x20 	KECCAK256 	Compute Keccak-256 hash
       // 0x21 - 0x2f 	Unused / Invalid
    case 0x30: // ADDRESS 	Get address of currently executing account
      state -> stack[state -> stack_pointer++] = state -> address;
      break;
    case 0x31: // BALANCE 	Get balance of the given account
      a = state -> stack[--state -> stack_pointer];
      //TODO: Implement getting balance of given account
      break;
    case 0x32: // ORIGIN 	Get execution origination address
      state -> stack[state -> stack_pointer++] = state -> origin;
      break;
    case 0x33: // CALLER 	Get caller address
      state -> stack[state -> stack_pointer++] = state -> caller;
      break;
    case 0x34: // CALLVALUE 	Get deposited value by the instruction/transaction responsible for this execution
      state -> stack[state -> stack_pointer++] = state -> value;
      break;
    case 0x35: // CALLDATALOAD 	Get input data of current environment
      a = state -> stack[--state -> stack_pointer];
      state -> stack[state -> stack_pointer++] = state -> calldata[a];
      break;
    case 0x36: // CALLDATASIZE 	Get size of input data in current environment
      state -> stack[state -> stack_pointer++] = state -> calldata_size;
      break;
    case 0x37: // CALLDATACOPY 	Copy input data in current environment to memory
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      c = state -> stack[--state -> stack_pointer];
      memcpy( & state -> memory[c], & state -> calldata[b], a);
      break;
    case 0x38: // CODESIZE 	Get size of code running in current environment
      state -> stack[state -> stack_pointer++] = state -> code_size;
      break;
    case 0x39: // CODECOPY 	Copy code running in current environment to memory
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      c = state -> stack[--state -> stack_pointer];
      memcpy( & state -> memory[c], & state -> code[b], a);
      break;
    case 0x3A: // GASPRICE 	Get price of gas in current environment
      state -> stack[state -> stack_pointer++] = state -> gas_price;
      break;
    case 0x3B: // EXTCODESIZE 	Get size of an account's code
      a = state -> stack[--state -> stack_pointer];
      //TODO: Implement getting size of code of an account
      break;
    case 0x3C: // EXTCODECOPY 	Copy an account's code to memory
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      c = state -> stack[--state -> stack_pointer];
      //TODO: Implement copying code of an account to memory
      break;
//0x3d 	RETURNDATASIZE 	Pushes the size of the return data buffer onto the stack
//0x3e 	RETURNDATACOPY 	Copies data from the return data buffer to memory
//0x3f 	EXTCODEHASH 	Returns the keccak256 hash of a contract's code
    case 0x40: // BLOCKHASH 	Get the hash of one of the 256 most recent complete blocks
      a = state -> stack[--state -> stack_pointer];
      //TODO: Implement getting the hash of one of the 256 most recent complete blocks
      break;
    case 0x41: // COINBASE 	Get the block's beneficiary address
      state -> stack[state -> stack_pointer++] = state -> coinbase;
      break;
    case 0x42: // TIMESTAMP 	Get the block's timestamp
      state -> stack[state -> stack_pointer++] = state -> timestamp;
      break;
    case 0x43: // NUMBER 	Get the block's number
      state -> stack[state -> stack_pointer++] = state -> block_number;
      break;
    case 0x44: // DIFFICULTY 	Get the block's difficulty ??? Could be 44	PREVRANDAO, randomnes
      state -> stack[state -> stack_pointer++] = state -> difficulty;
      break;
    case 0x45: // GASLIMIT 	Get the block's gas limit
      state -> stack[state -> stack_pointer++] = state -> gas_limit;
      break;
//46	CHAINID 	Returns the current chain’s EIP-155 unique identifier
//47	SELFBALANCE	.	address(this).balance		balance of executing contract
//0x47 - 0x4f 	??? Unused
    case 0x48: // BASEFEE 	Returns the value of the base fee of the current block it is executing in
      //TODO
      break;
//49-4F	invalid		
    case 0x50: // POP 	Remove word from stack
      state -> stack_pointer--;
      break;
    case 0x51: // MLOAD 	Load word from memory
      a = state -> stack[--state -> stack_pointer];
      state -> stack[state -> stack_pointer++] = state -> memory[a];
      break;
    case 0x52: // MSTORE 	Save word to memory
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      state -> memory[a] = b;
      break;
    case 0x53: // MSTORE8 	Save byte to memory
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      state -> memory[a] = (state -> memory[a] & 0xFFFFFF00) | (b & 0xFF);
      break;
    case 0x54: // SLOAD 	Load word from storage
      a = state -> stack[--state -> stack_pointer];
      state -> stack[state -> stack_pointer++] = state -> storage[a];
      break;
    case 0x55: // SSTORE 	Save word to storage
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      state -> storage[a] = b;
      break;
    case 0x56: // JUMP 	Alter the program counter
      a = state -> stack[--state -> stack_pointer];
      //TODO: Implement jump to destination
      break;
    case 0x57: // JUMPI 	Conditionally alter the program counter
      a = state -> stack[--state -> stack_pointer];
      b = state -> stack[--state -> stack_pointer];
      if (b != 0) {
        //TODO: Implement jump to destination
      }
      break;
    case 0x58: // GETPC 	Get the value of the program counter prior to the increment
      state -> stack[state -> stack_pointer++] = pc;
      break;
    case 0x59: // MSIZEMSIZE 	Get the size of active memory in bytes
      state -> stack[state -> stack_pointer++] = state -> memory_size;
      break;
    case 0x5A: // GAS 	Get the amount of available gas, including the corresponding reduction for the cost of this instruction
      state -> stack[state -> stack_pointer++] = state -> gas;
      break;
    case 0x5B: // JUMPDEST 	Mark a valid destination for jumps
      //TODO: Implement marking a valid destination for jumps
      break;
/* TODO
0x5c - 0x5f 	Invalid/Unused			
60	PUSH1	3	.	uint8		push 1-byte value onto stack	
61	PUSH2	3	.	uint16		push 2-byte value onto stack	
62	PUSH3	3	.	uint24		push 3-byte value onto stack	
63	PUSH4	3	.	uint32		push 4-byte value onto stack	
64	PUSH5	3	.	uint40		push 5-byte value onto stack	
65	PUSH6	3	.	uint48		push 6-byte value onto stack	
66	PUSH7	3	.	uint56		push 7-byte value onto stack	
67	PUSH8	3	.	uint64		push 8-byte value onto stack	
68	PUSH9	3	.	uint72		push 9-byte value onto stack	
69	PUSH10	3	.	uint80		push 10-byte value onto stack	
6A	PUSH11	3	.	uint88		push 11-byte value onto stack	
6B	PUSH12	3	.	uint96		push 12-byte value onto stack	
6C	PUSH13	3	.	uint104		push 13-byte value onto stack	
6D	PUSH14	3	.	uint112		push 14-byte value onto stack	
6E	PUSH15	3	.	uint120		push 15-byte value onto stack	
6F	PUSH16	3	.	uint128		push 16-byte value onto stack	
70	PUSH17	3	.	uint136		push 17-byte value onto stack	
71	PUSH18	3	.	uint144		push 18-byte value onto stack	
72	PUSH19	3	.	uint152		push 19-byte value onto stack	
73	PUSH20	3	.	uint160		push 20-byte value onto stack	
74	PUSH21	3	.	uint168		push 21-byte value onto stack	
75	PUSH22	3	.	uint176		push 22-byte value onto stack	
76	PUSH23	3	.	uint184		push 23-byte value onto stack	
77	PUSH24	3	.	uint192		push 24-byte value onto stack	
78	PUSH25	3	.	uint200		push 25-byte value onto stack	
79	PUSH26	3	.	uint208		push 26-byte value onto stack	
7A	PUSH27	3	.	uint216		push 27-byte value onto stack	
7B	PUSH28	3	.	uint224		push 28-byte value onto stack	
7C	PUSH29	3	.	uint232		push 29-byte value onto stack	
7D	PUSH30	3	.	uint240		push 30-byte value onto stack	
7E	PUSH31	3	.	uint248		push 31-byte value onto stack	
7F	PUSH32	3	.	uint256		push 32-byte value onto stack	
80	DUP1	3	a	a, a		clone 1st value on stack	
81	DUP2	3	_, a	a, _, a		clone 2nd value on stack	
82	DUP3	3	_, _, a	a, _, _, a		clone 3rd value on stack	
83	DUP4	3	_, _, _, a	a, _, _, _, a		clone 4th value on stack	
84	DUP5	3	..., a	a, ..., a		clone 5th value on stack	
85	DUP6	3	..., a	a, ..., a		clone 6th value on stack	
86	DUP7	3	..., a	a, ..., a		clone 7th value on stack	
87	DUP8	3	..., a	a, ..., a		clone 8th value on stack	
88	DUP9	3	..., a	a, ..., a		clone 9th value on stack	
89	DUP10	3	..., a	a, ..., a		clone 10th value on stack	
8A	DUP11	3	..., a	a, ..., a		clone 11th value on stack	
8B	DUP12	3	..., a	a, ..., a		clone 12th value on stack	
8C	DUP13	3	..., a	a, ..., a		clone 13th value on stack	
8D	DUP14	3	..., a	a, ..., a		clone 14th value on stack	
8E	DUP15	3	..., a	a, ..., a		clone 15th value on stack	
8F	DUP16	3	..., a	a, ..., a
*/
    case 0x90: // SWAP1 	Exchange 1st and 2nd stack items
      a = state -> stack[state -> stack_pointer - 1];
      state -> stack[state -> stack_pointer - 1] = state -> stack[state -> stack_pointer - 2];
      state -> stack[state -> stack_pointer - 2] = a;
      break;
    case 0x91: // SWAP2 	Exchange 1st and 3rd stack items
      a = state -> stack[state -> stack_pointer - 1];
      b = state -> stack[state -> stack_pointer - 2];
      state -> stack[state -> stack_pointer - 1] = state -> stack[state -> stack_pointer - 3];
      state -> stack[state -> stack_pointer - 2] = b;
      state -> stack[state -> stack_pointer - 3] = a;
      break;
    case 0x92: // SWAP3 	Exchange 1st and 4th stack items
      a = state -> stack[state -> stack_pointer - 1];
      b = state -> stack[state -> stack_pointer - 2];
      c = state -> stack[state -> stack_pointer - 3];
      state -> stack[state -> stack_pointer - 1] = state -> stack[state -> stack_pointer - 4];
      state -> stack[state -> stack_pointer - 2] = c;
      state -> stack[state -> stack_pointer - 3] = b;
      state -> stack[state -> stack_pointer - 4] = a;
      break;
    case 0x93: // SWAP4 	Exchange 1st and 5th stack items
      a = state -> stack[state -> stack_pointer - 1];
      b = state -> stack[state -> stack_pointer - 2];
      c = state -> stack[state -> stack_pointer - 3];
      d = state -> stack[state -> stack_pointer - 4];
      state -> stack[state -> stack_pointer - 1] = state -> stack[state -> stack_pointer - 5];
      state -> stack[state -> stack_pointer - 2] = d;
      state -> stack[state -> stack_pointer - 3] = c;
      state -> stack[state -> stack_pointer - 4] = b;
      state -> stack[state -> stack_pointer - 5] = a;
      break;
    case 0x94: // SWAP5 	Exchange 1st and 6th stack items
      a = state -> stack[state -> stack_pointer - 1];
      b = state -> stack[state -> stack_pointer - 2];
      c = state -> stack[state -> stack_pointer - 3];
      d = state -> stack[state -> stack_pointer - 4];
      e = state -> stack[state -> stack_pointer - 5];
      state -> stack[state -> stack_pointer - 1] = state -> stack[state -> stack_pointer - 6];
      state -> stack[state -> stack_pointer - 2] = e;
      state -> stack[state -> stack_pointer - 3] = d;
      state -> stack[state -> stack_pointer - 4] = c;
      state -> stack[state -> stack_pointer - 5] = b;
      state -> stack[state -> stack_pointer - 6] = a;
      break;
    case 0x95: // SWAP6 	Exchange 1st and 7th stack items
      a = state -> stack[state -> stack_pointer - 1];
      b = state -> stack[state -> stack_pointer - 2];
      c = state -> stack[state -> stack_pointer - 3];
      d = state -> stack[state -> stack_pointer - 4];
      e = state -> stack[state -> stack_pointer - 5];
      f = state -> stack[state -> stack_pointer - 6];
      state -> stack[state -> stack_pointer - 1] = state -> stack[state -> stack_pointer - 7];
      state -> stack[state -> stack_pointer - 2] = f;
      state -> stack[state -> stack_pointer - 3] = e;
      state -> stack[state -> stack_pointer - 4] = d;
      state -> stack[state -> stack_pointer - 5] = c;
      state -> stack[state -> stack_pointer - 6] = b;
      state -> stack[state -> stack_pointer - 7] = a;
      break;
    case 96: // SWAP7 	Exchange 1st and 8th stack items
      a = state -> stack[state -> stack_pointer - 1];
      b = state -> stack[state -> stack_pointer - 2];
      c = state -> stack[state -> stack_pointer - 3];
      d = state -> stack[state -> stack_pointer - 4];
      e = state -> stack[state -> stack_pointer - 5];
      f = state -> stack[state -> stack_pointer - 6];
      g = state -> stack[state -> stack_pointer - 7];
      state -> stack[state -> stack_pointer - 1] = state -> stack[state -> stack_pointer - 8];
      state -> stack[state -> stack_pointer - 2] = g;
      state -> stack[state -> stack_pointer - 3] = f;
      state -> stack[state -> stack_pointer - 4] = e;
      state -> stack[state -> stack_pointer - 5] = d;
      state -> stack[state -> stack_pointer - 6] = c;
      state -> stack[state -> stack_pointer - 7] = b;
      state -> stack[state -> stack_pointer - 8] = a;
      break;
    case 0x97: // SWAP8 	Exchange 1st and 9th stack items
      a = state -> stack[state -> stack_pointer - 1];
      b = state -> stack[state -> stack_pointer - 2];
      c = state -> stack[state -> stack_pointer - 3];
      d = state -> stack[state -> stack_pointer - 4];
      e = state -> stack[state -> stack_pointer - 5];
      f = state -> stack[state -> stack_pointer - 6];
      g = state -> stack[state -> stack_pointer - 7];
      h = state -> stack[state -> stack_pointer - 8];
      state -> stack[state -> stack_pointer - 1] = state -> stack[state -> stack_pointer - 9];
      state -> stack[state -> stack_pointer - 2] = h;
      state -> stack[state -> stack_pointer - 3] = g;
      state -> stack[state -> stack_pointer - 4] = f;
      state -> stack[state -> stack_pointer - 5] = e;
      state -> stack[state -> stack_pointer - 6] = d;
      state -> stack[state -> stack_pointer - 7] = c;
      state -> stack[state -> stack_pointer - 8] = b;
      state -> stack[state -> stack_pointer - 9] = a;
      break;
    case 0x98: // SWAP9
      break;
    case 0x99: // SWAP10
      break;
    case 0x9A: // SWAP11
      break;
    case 0x9B: // SWAP12
      break;
    case 0x9C: // SWAP13
      break;
    case 0x9D: // SWAP14
      break;
    case 0x9E: // SWAP15
      //....
      break;
    case 0x9F: // SWAP16
      a = state -> stack[state -> stack_pointer - 1];
      b = state -> stack[state -> stack_pointer - 2];
      c = state -> stack[state -> stack_pointer - 3];
      d = state -> stack[state -> stack_pointer - 4];
      e = state -> stack[state -> stack_pointer - 5];
      f = state -> stack[state -> stack_pointer - 6];
      g = state -> stack[state -> stack_pointer - 7];
      h = state -> stack[state -> stack_pointer - 8];
      i = state -> stack[state -> stack_pointer - 9];
      j = state -> stack[state -> stack_pointer - 10];
      k = state -> stack[state -> stack_pointer - 11];
      l = state -> stack[state -> stack_pointer - 12];
      m = state -> stack[state -> stack_pointer - 13];
      n = state -> stack[state -> stack_pointer - 14];
      o = state -> stack[state -> stack_pointer - 15];
      p = state -> stack[state -> stack_pointer - 16];
      state -> stack[state -> stack_pointer - 1] = p;
      state -> stack[state -> stack_pointer - 2] = o;
      state -> stack[state -> stack_pointer - 3] = n;
      state -> stack[state -> stack_pointer - 4] = m;
      state -> stack[state -> stack_pointer - 5] = l;
      state -> stack[state -> stack_pointer - 6] = k;
      state -> stack[state -> stack_pointer - 7] = j;
      state -> stack[state -> stack_pointer - 8] = i;
      state -> stack[state -> stack_pointer - 9] = h;
      state -> stack[state -> stack_pointer - 8] = i;
      state -> stack[state -> stack_pointer - 9] = h;
      state -> stack[state -> stack_pointer - 10] = g;
      state -> stack[state -> stack_pointer - 11] = f;
      state -> stack[state -> stack_pointer - 12] = e;
      state -> stack[state -> stack_pointer - 13] = d;
      state -> stack[state -> stack_pointer - 14] = c;
      state -> stack[state -> stack_pointer - 15] = b;
      state -> stack[state -> stack_pointer - 16] = a;
      break;
/*
A0	LOG0	A8	ost, len	.		LOG0(memory[ost:ost+len-1])	
A1	LOG1	A8	ost, len, topic0	.		LOG1(memory[ost:ost+len-1], topic0)	
A2	LOG2	A8	ost, len, topic0, topic1	.		LOG1(memory[ost:ost+len-1], topic0, topic1)	
A3	LOG3	A8	ost, len, topic0, topic1, topic2	.		LOG1(memory[ost:ost+len-1], topic0, topic1, topic2)	
A4	LOG4	A8	ost, len, topic0, topic1, topic2, topic3	.		LOG1(memory[ost:ost+len-1], topic0, topic1, topic2, topic3)	
A5-EF	invalid						
0xa5 - 0xaf 	Unused 	- 		
0xb0 	JUMPTO 	Tentative libevmasm has different numbers 	EIP 615 	
0xb1 	JUMPIF 	Tentative 	EIP 615 	
0xb2 	JUMPSUB 	Tentative 	EIP 615 	
0xb4 	JUMPSUBV 	Tentative 	EIP 615 	
0xb5 	BEGINSUB 	Tentative 	EIP 615 	
0xb6 	BEGINDATA 	Tentative 	EIP 615 	
0xb8 	RETURNSUB 	Tentative 	EIP 615 	
0xb9 	PUTLOCAL 	Tentative 	EIP 615 	
0xba 	GETLOCAL 	Tentative 	EIP 615 	
0xbb - 0xe0 	Unused 	- 		
0xe1 	SLOADBYTES 	Only referenced in pyethereum 	- 	-
0xe2 	SSTOREBYTES 	Only referenced in pyethereum 	- 	-
0xe3 	SSIZE
F0	CREATE	A9	val, ost, len	addr		addr = keccak256(rlp([address(this), this.nonce]))	
F1	CALL	AA	gas, addr, val, argOst, argLen, retOst, retLen	success	mem[retOst:retOst+retLen-1] := returndata		
F2	CALLCODE	AA	gas, addr, val, argOst, argLen, retOst, retLen	success	mem[retOst:retOst+retLen-1] = returndata	same as DELEGATECALL, but does not propagate original msg.sender and msg.value	
F3	RETURN	0*	ost, len	.		return mem[ost:ost+len-1]	
F4	DELEGATECALL	AA	gas, addr, argOst, argLen, retOst, retLen	success	mem[retOst:retOst+retLen-1] := returndata		
F5	CREATE2	A9	val, ost, len, salt	addr		addr = keccak256(0xff ++ address(this) ++ salt ++ keccak256(mem[ost:ost+len-1]))[12:]	
F6-F9	invalid						
FA	STATICCALL	AA	gas, addr, argOst, argLen, retOst, retLen	success	mem[retOst:retOst+retLen-1] := returndata		
FB	invalid	
0xfc 	TXEXECGAS 	Not in yellow paper FIXME					
FD	REVERT	0*	ost, len	.		revert(mem[ost:ost+len-1])	
FE	INVALID	AF			designated invalid opcode - EIP-141		
FF	SELFDESTRUCT	AB	addr
*/
    default:
      printf("Unknown opcode: %d\n", opcode);
    }
  }
}

int main() {
  evm_state state;
  state.stack_pointer = 0;
  state.memory_size = 0;
  state.balance = 100;

  // load contract bytecode from file or source
  const char * contract_bytecode = load_contract_from_file("smart_contract.bin");

  int contract_bytecode_size = sizeof(contract_bytecode);
  load_smart_contract( & state, contract_bytecode, contract_bytecode_size);

  // load contract into virtual machine's memory
  load_contract_to_memory( & state, contract_bytecode);

  // execute the contract
  execute_contract( & state, contract_bytecode);

  return 0;
}

/* 
load_contract(): function takes a smart contract bytecode and loads it into the virtual machine's memory, preparing it for execution.

call_function(): function takes a contract address, function signature, and input parameters and sends a message to the virtual machine to call the specified function on the smart contract.

get_storage(): function takes a contract address and a storage location and returns the value stored at that location.

set_storage(): function takes a contract address, a storage location, and a value, and sets the value at the specified storage location.

get_balance(): function takes an address and returns the balance of the account associated with that address.

transfer_value(): function takes an address, an amount, and a sender address and transfers the specified amount of value from the sender to the address.

emit_event(): function takes an event signature and a set of event data, and emits an event to the blockchain.

get_event(): function takes an event signature and a gets event data.

get_code(): function takes a contract address and returns the bytecode of the smart contract at that address.

get_return_data(): function retrieves the output data of the last executed smart contract function call.

create_contract(): function ttakes bytecode, a value, and a sender address and creates a new contract on the blockchain.

delete_contract(): function takes a contract address and deletes the smart contract at that address.
*/
