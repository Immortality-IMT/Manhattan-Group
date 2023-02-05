/*
C code defines a virtual machine to execute opcodes, specifically for smart contract execution. The opcodes that are executed by the VM are encoded in bytecode, which is a low-level, binary representation of the instructions that the VM should perform. The "execute_contract" function takes in bytecode as input and uses a loop and a switch statement to execute the corresponding opcode. The struct "evm_state" contains several arrays for storing data, such as the stack and memory, as well as several integer variables for storing information such as the stack pointer and balance. It does not access the blockchain, and it doesn't handle the interaction between smart contracts. Requires evm compatible bytecode to execute, paste it in smart_contract.bin in the currency directory. Get the bytecode from https://imt.cx/solc/ to test this.

Compiler with: gcc -g -o vm vm.c -lm

/// Note: The instruction set is not implemented ///

https://imt.cx/solc/
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

#define MAX_STACK_SIZE 1024 //to be removed in dynamic allocation
#define MAX_MEMORY_SIZE 256000 //to be removed

typedef struct {
    int stack_pointer; //keep track of the current position in the stack array
    int pc; //program counter, current position in the bytecode being executed.
    int *stack; //store values that are pushed and popped during the execution
    unsigned char *memory; //store values that are read or written during the execution
    unsigned char *contract_bytecode; //store the bytecode of the contract
    int memory_size;
    size_t code_size;
    int gas; //price of execution
    int balance; //of the contract
    int address; //address of the contract

//Values required for compilation but mostly not required
  int origin;
  int caller;
  int * calldata;
  int calldata_size;
  int * code;
  int gas_price;
  int gas_limit;
  int coinbase;
  int timestamp;
  int block_number;
  int difficulty;
  int value;
  int storage[MAX_MEMORY_SIZE];

} evm_state;

void load_contract_from_file(const char * file_path, evm_state * state) {

  // Open the file
  FILE * fp = fopen(file_path, "r");
  if (!fp) {
    printf("Error: unable to open file %s\n", file_path);
    return;
  }

  // Determine the size of the file
  fseek(fp, 0, SEEK_END);
  state->code_size = ftell(fp);
  rewind(fp);

  // Allocate memory for the bytecode
  state->contract_bytecode = (unsigned char * ) malloc(state->code_size + 1);
  if (!state->contract_bytecode) {
    printf("Error: unable to allocate memory for bytecode\n");
    fclose(fp);
    return;
  }

  // Read the bytecode from the file
  size_t bytes_read = fread(state->contract_bytecode, sizeof(char), state->code_size, fp);
  if (bytes_read != state->code_size) {
    printf("Error: unexpected number of bytes read\n");
    free(state->contract_bytecode);
    fclose(fp);
    return;
  }

  // Null-terminate the bytecode
  state->contract_bytecode[state->code_size] = '\0';

  // Close the file
  fclose(fp);
}

void run_evm(evm_state * state) {
  state->pc = 0;

  int a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p;

    unsigned char opcode[2];

  while (state->pc < state->code_size) {

    opcode[0] = state->contract_bytecode[state->pc];
    opcode[1] = state->contract_bytecode[state->pc + 1];
    opcode[2] = '\0';

    printf("%s - ", opcode);

   switch (opcode[0]) { //unsigned char opcode
      case '0': //[0] first hex literal, then nested [1] second 00, tolower ff


        switch (opcode[1]) { //STOP
            case '0': // STOP
                printf("STOP\n");
                return;
            break;
            case '1': // ADD 	Addition operation
                b = state->stack[--state->stack_pointer];
                a = state->stack[--state->stack_pointer];
                if (state->stack_pointer >= 0 && state->stack_pointer < MAX_STACK_SIZE) {
                state->stack[state->stack_pointer++] = a + b;
                printf("a: %d, b: %d, stack: %d\n", a, b, state->stack[state->stack_pointer - 1]);
                }
            break;
            case '2': // MUL 	Multiplication operation
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                state -> stack[state -> stack_pointer++] = a * b;
            break;
            case '3': // SUB 	Subtraction operation
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                state -> stack[state -> stack_pointer++] = a - b;
            break;
            case '4': // DIV 	Integer division operation
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                state -> stack[state -> stack_pointer++] = a / b;
            break;
            case '5': // SDIV 	Signed integer division operation (truncated)
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                state -> stack[state -> stack_pointer++] = ((a ^ INT_MIN) / (b ^ INT_MIN)) ^ INT_MIN;
            break;
            case '6': // MOD 	Modulo remainder operation
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                state -> stack[state -> stack_pointer++] = a % b;
            break;
            case '7': // SMOD 	Signed modulo remainder operation
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                state -> stack[state -> stack_pointer++] = ((a % b) + b) % b;
            break;
            case '8': // ADDMOD 	Modulo addition operation
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                c = state -> stack[--state -> stack_pointer];
                state -> stack[state -> stack_pointer++] = (a + b) % c;
            break;
            case '9': // MULMOD 	Modulo multiplication operation
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                c = state -> stack[--state -> stack_pointer];
                state -> stack[state -> stack_pointer++] = (a * b) % c;
            break;
            case 'a': // EXP 	Exponential operation
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                state -> stack[state -> stack_pointer++] = pow(a, b);
            break;
            case 'b': // SIGNEXTEND 	Extend length of two's complement signed integer
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                if ((a < 32) && (b & (1 << (a - 1)))) {
                    state -> stack[state -> stack_pointer++] = b | (0xffffffff << a);
                } else {
                    state -> stack[state -> stack_pointer++] = b;
                }
            break;
            case 'c': //Unsed/Invalid
                printf("0C - Unsed/Invalid\n");
            break;
            case 'd': //Unsed/Invalid
                printf("0D - Unused/Invalid\n");
            break;
            case 'e': //Unsed/Invalid
                printf("0E - Unused/Invalid\n");
            break;
            case 'f':  //Unsed/Invalid
                printf("0F - Unused/Invalid\n");
            break;
        }


      break; //0
      case '1': //1


        switch (opcode[1]) {
            case '0': // LT 	Less-than comparison
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                state -> stack[state -> stack_pointer++] = (a < b) ? 1 : 0;
            break;
            case '1':  // GT 	Greater-than comparison
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                state -> stack[state -> stack_pointer++] = (a > b) ? 1 : 0;
            break;
            case '2': // SLT 	Signed less-than comparison
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                state -> stack[state -> stack_pointer++] = (a < b) ? 1 : 0;
            break;
            case '3':  // SGT 	Signed greater-than comparison
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                state -> stack[state -> stack_pointer++] = (a > b) ? 1 : 0;
            break;
            case '4': // EQ 	Equality comparison
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                state -> stack[state -> stack_pointer++] = (a == b) ? 1 : 0;
            break;
            case '5': // ISZERO 	Simple not operator
                a = state -> stack[--state -> stack_pointer];
                state -> stack[state -> stack_pointer++] = (a == 0) ? 1 : 0;
            break;
            case '6': // AND 	Bitwise AND operation
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                state -> stack[state -> stack_pointer++] = a & b;
            break;
            case '7': // OR 	Bitwise OR operation
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                state -> stack[state -> stack_pointer++] = a | b;
            break;
            case '8': // XOR 	Bitwise XOR operation
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                state -> stack[state -> stack_pointer++] = a ^ b;
            break;
            case '9': // NOT 	Bitwise NOT operation
                a = state -> stack[--state -> stack_pointer];
                state -> stack[state -> stack_pointer++] = ~a;
            break;
            case 'a':  // BYTE 	Retrieve single byte from word
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                state -> stack[state -> stack_pointer++] = (b >> (8 * a)) & 0xFF;
            break;
            case 'b': // SHL 	    Shift Left
                printf("SHL todo\n");
            break;
            case 'c': // SHR 	    Logical Shift Right
                printf("SHR todo\n");
            break;
            case 'd': // SAR 	    Arithmetic Shift Right
                printf("SAR todo\n");
            break;
            case 'e': //Unsed/Invalid
                printf("1E - Unused/Invalid\n");
            break;
            case 'f': //Unsed/Invalid
                printf("1F - Unused/Invalid\n");
            break;
        }


      break; //1
      case '2': //2


        switch (opcode[1]) {
            case '0': // KECCAK256 	Compute Keccak-256 hash
                printf("KECCAK256 todo\n");
            break;
            case '1':
                printf("21 - Unused/Invalid\n");
            break;
            case '2':
                printf("22 - Unused/Invalid\n");
            break;
            case '3':
                printf("23 - Unused/Invalid\n");
            break;
            case '4':
                printf("24 - Unused/Invalid\n");
            break;
            case '5':
                printf("25 - Unused/Invalid\n");
            break;
            case '6':
                printf("26 - Unused/Invalid\n");
            break;
            case '7':
                printf("27 - Unused/Invalid\n");
            break;
            case '8':
                printf("28 - Unused/Invalid\n");
            break;
            case '9':
                printf("29 - Unused/Invalid\n");
            break;
            case 'a':
                printf("2A - Unused/Invalid\n");
            break;
            case 'b':
                printf("2B - Unused/Invalid\n");
            break;
            case 'c':
                printf("2C - Unused/Invalid\n");
            break;
            case 'd':
                printf("2D - Unused/Invalid\n");
            break;
            case 'e':
                printf("2E - Unused/Invalid\n");
            break;
            case 'f':
                printf("2F - Unused/Invalid\n");
            break;
        }


      break; //2
      case '3': //3


        switch (opcode[1]) {
            case '0': // ADDRESS 	Get address of currently executing account
                state -> stack[state -> stack_pointer++] = state -> address;
            break;
            case '1': // BALANCE 	Get balance of the given account
                a = state -> stack[--state -> stack_pointer];
                //TODO: Implement getting balance of given account
            break;
            case '2': // ORIGIN 	Get execution origination address
                state -> stack[state -> stack_pointer++] = state -> origin;
            break;
            case '3': // CALLER 	Get caller address
                state -> stack[state -> stack_pointer++] = state -> caller;
            break;
            case '4': // CALLVALUE 	Get deposited value by the instruction/transaction responsible for this execution
                state -> stack[state -> stack_pointer++] = state -> value;
            break;
            case '5': // CALLDATALOAD 	Get input data of current environment
                a = state -> stack[--state -> stack_pointer];
                state -> stack[state -> stack_pointer++] = state -> calldata[a];
            break;
            case '6': // CALLDATASIZE 	Get size of input data in current environment
                state -> stack[state -> stack_pointer++] = state -> calldata_size;
            break;
            case '7': // CALLDATACOPY 	Copy input data in current environment to memory
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                c = state -> stack[--state -> stack_pointer];
                memcpy( & state -> memory[c], & state -> calldata[b], a);
            break;
            case '8': // CODESIZE 	Get size of code running in current environment
                state -> stack[state -> stack_pointer++] = state -> code_size;
            break;
            case '9': // CODECOPY 	Copy code running in current environment to memory
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                c = state -> stack[--state -> stack_pointer];
                memcpy( & state -> memory[c], & state -> code[b], a);
            break;
            case 'a': // GASPRICE 	Get price of gas in current environment
                state -> stack[state -> stack_pointer++] = state -> gas_price;
            break;
            case 'b': // EXTCODESIZE 	Get size of an account's code
                a = state -> stack[--state -> stack_pointer];
                //TODO: Implement getting size of code of an account
            break;
            case 'c': // EXTCODECOPY 	Copy an account's code to memory
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                c = state -> stack[--state -> stack_pointer];
                //TODO: Implement copying code of an account to memory
            break;
            case 'd': // RETURNDATASIZE 	Pushes the size of the return data buffer onto the stack
                printf("RETURNDATASIZE todo\n");
            break;
            case 'e': // RETURNDATACOPY 	Copies data from the return data buffer to memory
                printf("RETURNDATACOPY todo\n");
            break;
            case 'f': // EXTCODEHASH 	Returns the keccak256 hash of a contract's code
                printf("EXTCODEHASH todo\n");
            break;
        }


      break;
      case '4': // 4


        switch (opcode[1]) {
            case '0': // BLOCKHASH 	Get the hash of one of the 256 most recent complete blocks
                a = state -> stack[--state -> stack_pointer];
                //TODO: Implement getting the hash of one of the 256 most recent complete blocks
            break;
            case '1': // COINBASE 	Get the block's beneficiary address
                state -> stack[state -> stack_pointer++] = state -> coinbase;
            break;
            case '2': // TIMESTAMP 	Get the block's timestamp
                state -> stack[state -> stack_pointer++] = state -> timestamp;
            break;
            case '3': // NUMBER 	Get the block's number
                state -> stack[state -> stack_pointer++] = state -> block_number;
            break;
            case '4': // DIFFICULTY 	Get the block's difficulty ??? Could be 44	PREVRANDAO, randomnes
                state -> stack[state -> stack_pointer++] = state -> difficulty;
            break;
            case '5': // GASLIMIT 	Get the block's gas limit
                state -> stack[state -> stack_pointer++] = state -> gas_limit;
            break;
            case '6': //46	CHAINID 	Returns the current chain’s EIP-155 unique identifier
                printf("CHAINID  todo\n");
            break;
            case '7': //47	SELFBALANCE	.	address(this).balance		balance of executing contract
                printf("SELFBALANCE\n");
            break;
            case '8': // BASEFEE 	Returns the value of the base fee of the current block it is executing in
                printf("BASEFEE todo\n");
            break;
            case '9': // Invalid
                printf("push1\n");
            break;
            case 'a': // PUSH1
                printf("push1\n");
            break;
            case 'b': // PUSH1
                printf("push1\n");
            break;
            case 'c': // PUSH1
                printf("push1\n");
            break;
            case 'd': // PUSH1
                printf("push1\n");
            break;
            case 'e': // PUSH1
                printf("push1\n");
            break;
            case 'f': // PUSH1
                printf("push1\n");
            break;
        }
        break; //4


      case '5': // 5
        switch (opcode[1]) {


            case '0':// POP 	Remove word from stack
                state -> stack_pointer--;
            break;
            case '1': // MLOAD 	Load word from memory
                a = state -> stack[--state -> stack_pointer];
                state -> stack[state -> stack_pointer++] = state -> memory[a];
            break;
            case '2': // MSTORE 	Save word to memory
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                state -> memory[a] = b;
            break;
            case '3': // MSTORE8 	Save byte to memory
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                state -> memory[a] = (state -> memory[a] & 0xFFFFFF00) | (b & 0xFF);
            break;
            case '4': // SLOAD 	Load word from storage
                a = state -> stack[--state -> stack_pointer];
                state -> stack[state -> stack_pointer++] = state -> storage[a];
            break;
            case '5': // SSTORE 	Save word to storage
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                state -> storage[a] = b;
            break;
            case '6': // JUMP 	Alter the program counter
                a = state -> stack[--state -> stack_pointer];
                //TODO: Implement jump to destination
            break;
            case '7': // JUMPI 	Conditionally alter the program counter
                a = state -> stack[--state -> stack_pointer];
                b = state -> stack[--state -> stack_pointer];
                if (b != 0) {
                    //TODO: Implement jump to destination
                }
            break;
            case '8': // GETPC 	Get the value of the program counter prior to the increment
                state -> stack[state -> stack_pointer++] = state->pc;
            break;
            case '9': // MSIZEMSIZE 	Get the size of active memory in bytes
                state -> stack[state -> stack_pointer++] = state -> memory_size;
            break;
            case 'a':  // GAS 	Get the amount of available gas, including the corresponding reduction for the cost of this instruction
                state -> stack[state -> stack_pointer++] = state -> gas;
            break;
            case 'b': /// JUMPDEST 	Mark a valid destination for jumps
                //TODO: Implement marking a valid destination for jumps
            break;
            case 'c': // Invalid
                printf("Invalid\n");
            break;
            case 'd': // Invalid
                printf("Invalid\n");
            break;
            case 'e': // Invalid
                printf("Invalid\n");
            break;
            case 'f': // Invalid
                printf("Invalid\n");
            break;
        }
        break;


      case '6': // 6
        switch (opcode[1]) {


            case '0': // 60	PUSH1	3	.	uint8		push 1-byte value onto stack
                printf("PUSH1 todo\n");
            break;
            case '1': // 61	PUSH2	3	.	uint16		push 2-byte value onto stack	
                printf("PUSH2 todo\n");
            break;
            case '2': // 62	PUSH3	3	.	uint24		push 3-byte value onto stack	
                printf("PUSH3 todo\n");
            break;
            case '3': // 63	PUSH5	3	.	uint40		push 5-byte value onto stack
                printf("PUSH3 todo\n");
            break;
            case '4': // 64	PUSH6	3	.	uint48		push 6-byte value onto stack
                printf("PUSH4 todo\n");
            break;
            case '5': // 65	PUSH7	3	.	uint56		push 7-byte value onto stack
                printf("PUSH5 todo\n");
            break;
            case '6': // 66	PUSH8	3	.	uint64		push 8-byte value onto stack
                printf("PUSH6 todo\n");
            break;
            case '7': // 67	PUSH9	3	.	uint72		push 9-byte value onto stack
                printf("PUSH7 todo\n");
            break;
            case '8': // 68	PUSH todo0	3	.	uint80		push 10-byte value onto stack	
                printf("PUSH8 todo\n");
            break;
            case '9': // 69	PUSH todo0	3	.	uint80		push 10-byte value onto stack	
                printf("PUSH9 todo\n");
            break;
            case 'a': // 6A	PUSH todo1	3	.	uint88		push 11-byte value onto stack
                printf("PUSH10 todo\n");
            break;
            case 'b': // 6B	PUSH todo2	3	.	uint96		push 12-byte value onto stack
                printf("PUSH11 todo\n");
            break;
            case 'c': // 6C	PUSH todo3	3	.	uint104		push 13-byte value onto stack
                printf("PUSH12 todo\n");
            break;
            case 'd': // 6D	PUSH todo4	3	.	uint112		push 14-byte value onto stack
                printf("PUSH13 todo\n");
            break;
            case 'e': // 6E	PUSH todo5	3	.	uint120		push 15-byte value onto stack
                printf("PUSH14 todo\n");
            break;
            case 'f': // 6F	PUSH todo6	3	.	uint128		push 16-byte value onto stack	
                printf("PUSH15 todo\n");
            break;
        }
        break; //6


      case '7': // 7
        switch (opcode[1]) {

            case '0': // 70	PUSH17	3	.	uint136		push 17-byte value onto stack
                printf("Push 17 todo\n");
            break;
            case '1': // 71	PUSH18	3	.	uint144		push 18-byte value onto stack
                printf("PUSH 18 todo\n");
            break;
            case '2': // 72	PUSH19	3	.	uint152		push 19-byte value onto stack
                printf("PUSH 19 todo\n");
            break;
            case '3': // 73	PUSH20	3	.	uint160		push 20-byte value onto stack
                printf("PUSH 20 todo\n");
            break;
            case '4': // 74	PUSH21	3	.	uint168		push 21-byte value onto stack	
                printf("PUSH 21 todo\n");
            break;
            case '5': // 75	PUSH22	3	.	uint176		push 22-byte value onto stack
                printf("PUSH 22 todo\n");
            break;
            case '6': // 76	PUSH23	3	.	uint184		push 23-byte value onto stack
                printf("PUSH 23 todo\n");
            break;
            case '7': // 77	PUSH24	3	.	uint192		push 24-byte value onto stack
                printf("PUSH 24 todo\n");
            break;
            case '8': // 78	PUSH25	3	.	uint200		push 25-byte value onto stack
                printf("PUSH 25 todo\n");
            break;
            case '9': // 79	PUSH26	3	.	uint208		push 26-byte value onto stack
                printf("PUSH 26 todo\n");
            break;
            case 'a': // 7A	PUSH27	3	.	uint216		push 27-byte value onto stack
                printf("PUSH 27 todo\n");
            break;
            case 'b': // 7B	PUSH28	3	.	uint224		push 28-byte value onto stack
                printf("PUSH 28 todo\n");
            break;
            case 'c': // 7C	PUSH29	3	.	uint232		push 29-byte value onto stack
                printf("PUSH 29 todo\n");
            break;
            case 'd': // 7D	PUSH30	3	.	uint240		push 30-byte value onto stack
                printf("PUSH 30 todo\n");
            break;
            case 'e': // 7E	PUSH31	3	.	uint248		push 31-byte value onto stack
                printf("PUSH 31 todo\n");
            break;
            case 'f': // 7F	PUSH32	3	.	uint256		push 32-byte value onto stack	
                printf("PUSH 32 todo\n");
            break;
        }
        break;


      case '8': // 8
        switch (opcode[1]) {


            case '0': // 80	DUP1	3	a	a, a		clone 1st value on stack
                printf("DUP 1\n");
            break;
            case '1': // 81	DUP2	3	_, a	a, _, a		clone 2nd value on stack
                printf("DUP todo\n");
            break;
            case '2': // 82	DUP3	3	_, _, a	a, _, _, a		clone 3rd value on stack
                printf("DUP todo\n");
            break;
            case '3': // 83	DUP4	3	_, _, _, a	a, _, _, _, a		clone 4th value on stack
                printf("DUP todo\n");
            break;
            case '4': // 84	DUP5	3	..., a	a, ..., a		clone 5th value on stack
                printf("DUP todo\n");
            break;
            case '5': // 85	DUP6	3	..., a	a, ..., a		clone 6th value on stack
                printf("DUP todo\n");
            break;
            case '6': // 86	DUP7	3	..., a	a, ..., a		clone 7th value on stack
                printf("DUP todo\n");
            break;
            case '7': // 87	DUP8	3	..., a	a, ..., a		clone 8th value on stack
                printf("DUP todo\n");
            break;
            case '8': // 88	DUP9	3	..., a	a, ..., a		clone 9th value on stack
                printf("DUP todo\n");
            break;
            case '9': // 89	DUP10	3	..., a	a, ..., a		clone 10th value on stack
                printf("DUP todo\n");
            break;
            case 'a': // 8A	DUP11	3	..., a	a, ..., a		clone 11th value on stack
                printf("DUP todo\n");
            break;
            case 'b': // 8B	DUP12	3	..., a	a, ..., a		clone 12th value on stack
                printf("DUP todo\n");
            break;
            case 'c': // 8C	DUP13	3	..., a	a, ..., a		clone 13th value on stack
                printf("DUP todo\n");
            break;
            case 'd': // 8D	DUP14	3	..., a	a, ..., a		clone 14th value on stack
                printf("DUP todo\n");
            break;
            case 'e': // 8E	DUP15	3	..., a	a, ..., a		clone 15th value on stack
                printf("DUP todo\n");
            break;
            case 'f': // 8F	DUP16	3	..., a	a, ..., a
                printf("DUP todo\n");
            break;
        }
        break;


      case '9': // 9
        switch (opcode[1]) {


            case '0': /// SWAP1 	Exchange 1st and 2nd stack items
      a = state -> stack[state -> stack_pointer - 1];
      state -> stack[state -> stack_pointer - 1] = state -> stack[state -> stack_pointer - 2];
      state -> stack[state -> stack_pointer - 2] = a;
            break;
            case '1': // SWAP2 	Exchange 1st and 3rd stack items
      a = state -> stack[state -> stack_pointer - 1];
      b = state -> stack[state -> stack_pointer - 2];
      state -> stack[state -> stack_pointer - 1] = state -> stack[state -> stack_pointer - 3];
      state -> stack[state -> stack_pointer - 2] = b;
      state -> stack[state -> stack_pointer - 3] = a;
            break;
            case '2':  // SWAP3 	Exchange 1st and 4th stack items
      a = state -> stack[state -> stack_pointer - 1];
      b = state -> stack[state -> stack_pointer - 2];
      c = state -> stack[state -> stack_pointer - 3];
      state -> stack[state -> stack_pointer - 1] = state -> stack[state -> stack_pointer - 4];
      state -> stack[state -> stack_pointer - 2] = c;
      state -> stack[state -> stack_pointer - 3] = b;
      state -> stack[state -> stack_pointer - 4] = a;
            break;
            case '3': // SWAP4 	Exchange 1st and 5th stack items
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
            case '4': // SWAP5 	Exchange 1st and 6th stack items
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
            case '5': // SWAP6 	Exchange 1st and 7th stack items
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
            case '6': // SWAP7 	Exchange 1st and 8th stack items
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
            case '7': // SWAP8 	Exchange 1st and 9th stack items
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
            case '8': // SWAP9
                printf("Swap 9 todo\n");
            break;
            case '9': // SWAP10
                printf("Swap 10 todo\n");
            break;
            case 'a': // SWAP11
                printf("Swap 11 todo\n");
            break;
            case 'b': // SWAP12
                printf("Swap 12 todo\n");
            break;
            case 'c': // SWAP13
                printf("Swap 13 todo\n");
            break;
            case 'd': // SWAP14
                printf("Swap 14 todo\n");
            break;
            case 'e': // SWAP15
                printf("Swap 15 todo\n");
            break;
            case 'f':  // SWAP16
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
        }
        break;


      case 'a': //a
        switch (opcode[1]) {


            case '0': // A0	LOG0	A8	ost, len	.		LOG0(memory[ost:ost+len-1])	
                printf("LOG0 todo\n");
            break;
            case '1': // A1	LOG1	A8	ost, len, topic0	.		LOG1(memory[ost:ost+len-1], topic0)
                printf("LOG1 todo\n");
            break;
            case '2': // A2	LOG2	A8	ost, len, topic0, topic1	.		LOG1(memory[ost:ost+len-1], topic0, topic1)
                printf("LOG2 todo\n");
            break;
            case '3': // A3	LOG3	A8	ost, len, topic0, topic1, topic2	.		LOG1(memory[ost:ost+len-1], topic0, topic1, topic2)
                printf("LOG3 todo\n");
            break;
            case '4': // A4	LOG4	A8	ost, len, topic0, topic1, topic2, topic3	.		LOG1(memory[ost:ost+len-1], topic0, topic1, topic2, topic3)
                printf("LOG4 todo\n");
            break;
            case '5': // Unused
                printf("A5 Unused\n");
            break;
            case '6': // Unused
                printf("A6 Unused\n");
            break;
            case '7': // Unused
                printf("A7 Unused\n");
            break;
            case '8': // Unused
                printf("A8 Unused\n");
            break;
            case '9': // Unused
                printf("A9 Unused\n");
            break;
            case 'a': // Unused
                printf("AA Unused\n");
            break;
            case 'b': // Unused
                printf("AB Unused\n");
            break;
            case 'c': // Unused
                printf("AC Unused\n");
            break;
            case 'd': // Unused
                printf("AD Unused\n");
            break;
            case 'e': // Unused
                printf("AE Unused\n");
            break;
            case 'f': // Unused
                printf("AF Unused\n");
            break;
        }
        break; //a


      case 'b': //b
        switch (opcode[1]) { 


            case '0': // JUMPTO 	Tentative libevmasm has different numbers 	EIP 615 
                printf("JUMPTO todo\n");
            break;
            case '1': // JUMPIF 	Tentative 	EIP 615 
                printf("JUMPIF todo\n");
            break;
            case '2': // JUMPSUB 	Tentative 	EIP 615
                printf("JUMPSUB\n");
            break;
            case '3': // Unused
                printf("B3 Unused\n");
            break;
            case '4': // JUMPSUBV 	Tentative 	EIP 615 
                printf("JUMPSUBV\n");
            break;
            case '5': // BEGINSUB 	Tentative 	EIP 615 
                printf("BEGINSUB todo\n");
            break;
            case '6': // BEGINDATA 	Tentative 	EIP 615 	
                printf("BEGINDATA todo\n");
            break;
            case '7': // Unused
                printf("Unused\n");
            break;
            case '8': // RETURNSUB 	Tentative 	EIP 615
                printf("RETURNSUB todo\n");
            break;
            case '9': // PUTLOCAL 	Tentative 	EIP 615 	
                printf("PUTLOCAL todo\n");
            break;
            case 'a': // Unused
                printf("Unused\n");
            break;
            case 'b': // GETLOCAL 	Tentative 	EIP 615 
                printf("GETLOCAL todo\n");
            break;
            case 'c': // Unused
                printf("Unused\n");
            break;
            case 'd': // Unused
                printf("Unused\n");
            break;
            case 'e': // Unused
                printf("Unused\n");
            break;
            case 'f': // PUSH1
                printf("push1\n");
            break;
        }
        break; //b


      case 'c': // PUSH1
        switch (opcode[1]) { //STOP
            case '0': // STOP
                printf("STOP\n");
               // return;
            break;
            case '1': // ADD
                printf("push1\n");
            break;
            case '2': // PUSH1
                printf("push1\n");
            break;
            case '3': // PUSH1
                printf("push1\n");
            break;
            case '4': // PUSH1
                printf("push1\n");
            break;
            case '5': // PUSH1
                printf("push1\n");
            break;
            case '6': // PUSH1
                printf("push1\n");
            break;
            case '7': // PUSH1
                printf("push1\n");
            break;
            case '8': // PUSH1
                printf("push1\n");
            break;
            case '9': // PUSH1
                printf("push1\n");
            break;
            case 'a': // PUSH1
                printf("push1\n");
            break;
            case 'b': // PUSH1
                printf("push1\n");
            break;
            case 'c': // PUSH1
                printf("push1\n");
            break;
            case 'd': // PUSH1
                printf("push1\n");
            break;
            case 'e': // PUSH1
                printf("push1\n");
            break;
            case 'f': // PUSH1
                printf("push1\n");
            break;
        }
        break;


      case 'd': //d
        switch (opcode[1]) {

            case '0': // Unused
                printf("Unused\n");
            break;
            case '1': // Unused
                printf("push1\n");
            break;
            case '2': // PUSH1
                printf("push1\n");
            break;
            case '3': // PUSH1
                printf("push1\n");
            break;
            case '4': // PUSH1
                printf("push1\n");
            break;
            case '5': // PUSH1
                printf("push1\n");
            break;
            case '6': // PUSH1
                printf("push1\n");
            break;
            case '7': // PUSH1
                printf("push1\n");
            break;
            case '8': // PUSH1
                printf("push1\n");
            break;
            case '9': // PUSH1
                printf("push1\n");
            break;
            case 'a': // PUSH1
                printf("push1\n");
            break;
            case 'b': // PUSH1
                printf("push1\n");
            break;
            case 'c': // PUSH1
                printf("push1\n");
            break;
            case 'd': // PUSH1
                printf("push1\n");
            break;
            case 'e': // PUSH1
                printf("push1\n");
            break;
            case 'f': // PUSH1
                printf("push1\n");
            break;
        }
        break;


      case 'e':
        switch (opcode[1]) {


            case '0': // Invalid
                printf("E0\n");
            break;
            case '1': // SLOADBYTES 	Only referenced in pyethereum 	- 	-
                printf("SLOADBYTES todo\n");
            break;
            case '2': // SSTOREBYTES 	Only referenced in pyethereum 	- 	-
                printf("SSTOREBYTES todo\n");
            break;
            case '3': // SSIZE
                printf("SSIZE todo\n");
            break;
            case '4': // Invalid
                printf("E4 Invaid\n");
            break;
            case '5': // Invalid
                printf("E5 Invalid\n");
            break;
            case '6': // Invalid
                printf("E6 Invalid\n");
            break;
            case '7': // Invalid
                printf("E7 Invalid\n");
            break;
            case '8': // Invalid
                printf("E8 Invalid\n");
            break;
            case '9': // Invalid
                printf("E9 Invalid\n");
            break;
            case 'a': // Invalid
                printf("EA Invalid\n");
            break;
            case 'b': // Invalid
                printf("EB Invalid\n");
            break;
            case 'c': // Invalid
                printf("EC Invalid\n");
            break;
            case 'd': // Invalid
                printf("ED Invalid\n");
            break;
            case 'e': // Invalid
                printf("EE Invalid\n");
            break;
            case 'f': // Invalid
                printf("EF Invalid\n");
            break;
        }
        break;


     case 'f':
        switch (opcode[1]) {


            case '0': //CREATE	A9	val, ost, len	addr		addr = keccak256(rlp([address(this), this.nonce]))	
                printf("F0\n");
            break;
            case '1': //F1	CALL	AA	gas, addr, val, argOst, argLen, retOst, retLen	success	mem[retOst:retOst+retLen-1] := returndata	
                printf("F1\n");
            break;
            case '2': //F2	CALLCODE same as DELEGATECALL, but does not propagate original...
                printf("F2\n");
            break;
            case '3': //F3	RETURN	0*	ost, len	.		return mem[ost:ost+len-1]
                printf("F3\n");
            break;
            case '4': //F4	DELEGATECALL	AA	gas, addr, argOst, argLen, retOst, retLen	success	mem[retOst:retOst+retLen-1] := returndata
                printf("F4\n");
            break;
            case '5': //F5	CREATE2	A9	val, ost, len, salt	addr		addr = keccak256(0xff ++ address(th...
                printf("F5\n");
            break;
            case '6': //F6-F9	invalid		
                printf("Invalid\n");
            break;
            case '7': //F6-F9	invalid
                printf("Invalid\n");
            break;
            case '8': //F6-F9	invalid
                printf("Invalid\n");
            break;
            case '9': //F6-F9	invalid
                printf("Invalid\n");
            break;
            case 'a': //FA	STATICCALL	AA	gas, addr, argOst, argLen, retOst, retLen	success	mem[retOst:retOst+retLen-1] := returndata	
                printf("FA\n");
            break;
            case 'b': //FB	invalid	
                printf("FB Invalid/Unused\n");
            break;
            case 'c': //TXEXECGAS 	Not in yellow paper FIXME		
                printf("FC\n");
            break;
            case 'd': // FD	REVERT	0*	ost, len	.		revert(mem[ost:ost+len-1])	
                printf("FD\n");
            break;
            case 'e': //FE	INVALID	AF			designated invalid opcode - EIP-141	
                printf("FE\n");
            break;
            case 'f': //FF	SELFDESTRUCT	AB	addr
                printf("FF\n");
            break;
        }
        break;

      default:
           printf("Unknown opcode %s at instruction %d\n", opcode, state->pc);
        break;
    }
     state->pc += 2; // increment by 2 to skip to the next hex encoded byte
     //state->pc += 1;
  }
}

int main() {
  evm_state state;

    // load contract bytecode from file or source
    load_contract_from_file("smart_contract.bin", &state);

    printf("%s", state.contract_bytecode);

    // execute the contract
    run_evm(&state);

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

