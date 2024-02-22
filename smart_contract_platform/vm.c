/*  
  Implementation of a byecode executor
  https://www.evm.codes/

Not all commands are implemented, such as
 1) each case picks up the gas cost
 2) result return and error return
 3) blockchain specific commands
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include <inttypes.h>

//For mstore and mload
typedef struct {
    size_t size;
    uint8_t* data;
} Memory; 

typedef struct {
    int size;
    int top;
    int64_t* data;
    Memory* memory;    //MSTORE
    int64_t** storage; //SSTORE
} Stack;

Stack* stack_create(size_t initial_capacity) {
    Stack* stack = (Stack*) malloc(sizeof(Stack));
    if (stack == NULL) {
        perror("Failed to allocate memory for stack");
        exit(1);
    }

    stack->size = initial_capacity;
    stack->top = -1;
    stack->data = (int64_t*) malloc(initial_capacity * sizeof(int64_t));
    if (stack->data == NULL) {
        perror("Failed to allocate memory for stack data");
        free(stack);
        exit(1);
    }

    stack->memory = malloc(sizeof(Memory)); // Allocate memory for the Memory instance
    if (stack->memory == NULL) {
        perror("Failed to allocate memory for memory in stack");
        free(stack->data);
        free(stack);
        exit(1);
    }

    stack->memory->size = 32; // Initialize with 32 bytes //MSTORE
    stack->memory->data = (uint8_t*) malloc(stack->memory->size); //MSTORE
    stack->storage = malloc(sizeof(int64_t*) * (stack->memory->size / 8)); //SSTORE
    if (stack->storage == NULL || stack->memory->data == NULL) {
        perror("Failed to allocate memory");
        free(stack->memory->data);
        free(stack->memory);
        free(stack->data);
        free(stack);
        exit(1);
    }

    // Initialize the storage mapping with NULL pointers
    for (size_t i = 0; i < stack->memory->size / 8; ++i) {
        stack->storage[i] = NULL;
    }

    return stack;
}

void print_stack(Stack* stack) {
    printf("Stack:\n");
    for (int i = stack->top; i >= 0; i--) {
        printf("%ld ", stack->data[i]);
    }
    printf("\n");
}

int64_t** stack_get_storage_address(Stack* stack, int64_t key) {
    // Convert the 256-bit key to a 32-byte address
    uint8_t address[32];
    for (size_t i = 0; i < 32; ++i) {
        address[i] = (key >> (8 * (31 - i))) & 0xFF;
    }

    // Find the corresponding storage address
    int64_t** storage_address = &stack->storage[address[0]];
    int64_t* temp_address = NULL;
    for (size_t i = 1; i < 32; ++i) {
        if (*storage_address == NULL) {
            break;
        }
        temp_address = &(*storage_address)[address[i]];
        if (temp_address == NULL) {
            break;
        }
        storage_address = &temp_address;
    }

    return storage_address;
}

void stack_set_storage(Stack* stack, int64_t key, int64_t value) {
    int64_t** storage_address = stack_get_storage_address(stack, key);
    if (*storage_address == NULL) {
        *storage_address = malloc(sizeof(int64_t));
        if (*storage_address == NULL) {
            perror("Failed to allocate memory for storage value");
            exit(1);
        }
    }
    **storage_address = value;
}

//Pushes
void stack_push(Stack* stack, int64_t value) {
    if (stack->top >= stack->size - 1) {
        printf("Stack overflow! Increase the stack size.\n");
        exit(1);
    }
    stack->data[++stack->top] = value;
}

//Pops
int64_t stack_pop(Stack* stack) {
    if (stack->top < 0) {
        printf("POP: Stack underflow!\n");
        exit(1);
    }
    return stack->data[stack->top--];
}

//Dups
void stack_dup(Stack* stack, int x) {
    if (stack->top < (x - 1)) {
        printf("DUP%d: Stack underflow!\n", x);
        exit(1);
    }

    int64_t a[x]; // Temporary array to store duplicated values

    for (int i = 0; i < x; i++) {
        a[i] = stack->data[stack->top - i];
    }

    for (int i = x - 1; i >= 0; i--) { // Push values back onto the stack in reverse order
        stack_push(stack, a[i]);
    }
}

//SWAP
void stack_swap(Stack* stack, int x) {

    if (stack->top < x) {
        printf("SWAP%d: Stack underflow!\n", x);
        exit(1);
    }

    int64_t a[x + 1]; // Temporary array to store swapped values

    // Copy values from the stack to the temporary array
    for (int i = 0; i <= x; i++) {
        a[i] = stack->data[stack->top - i];
    }

    // Swap the values and put them back on the stack
    for (int i = 0; i <= x; i++) {
        stack->data[stack->top - i] = a[x - i];
    }
}

void stack_LOG(Stack* stack, int topic_count) {

    /*
    LOG0: Requires 2 elements on the stack (offset and size).
    LOG1: Requires 3 elements on the stack (offset, size, and 1 topic).
    LOG2: Requires 4 elements on the stack (offset, size, and 2 topics).
    LOG3: Requires 5 elements on the stack (offset, size, and 3 topics).
    LOG4: Requires 6 elements on the stack (offset, size, and 4 topics).
    */

    // Check if there are enough elements on the stack before popping
    if (stack->top + 1 < topic_count + 2) {  //init = -1
        printf("LOG%d: Stack underflow!\n", topic_count);
        exit(1);
    }

    int64_t offset = stack_pop(stack);
    int64_t size = stack_pop(stack);

    /* Check if the current execution context is from a STATICCALL (Byzantium fork)
    // For simplicity, let's assume it's not a STATICCALL
    if ( check if staticcall ) {
        printf("LOGx is not allowed in a STATICCALL context.\n");
        exit(1);
    } */

    // Read topics from the stack
    int64_t topics[topic_count];
    for (int i = 0; i < topic_count; i++) {
        if(topic_count != 0) { // log0 has no topics
        topics[i] = stack_pop(stack);
        }
    }

    // Read the memory content
    uint8_t* memory_data = stack->memory->data + offset;
    uint8_t* memory_end = memory_data + size;

    // Ensure memory range is valid
    if (memory_data < stack->memory->data || memory_end > stack->memory->data + stack->memory->size) {
        printf("Memory access out of bounds for LOGx.\n");
        exit(1);
    }

    // Log the data and topics (for simplicity, just print them)
    printf("LOGx (topics count: %d): Data:", topic_count);
    for (uint8_t* ptr = memory_data; ptr < memory_end; ptr++) {
        printf(" %02X", *ptr);
    }
    printf("\nTopics:");
    for (int i = 0; i < topic_count; i++) {
        printf(" %" PRId64, topics[i]);
    }
    printf("\n");
}

void stack_clear(Stack* stack) {
    stack->top = -1;
}

void stack_destroy(Stack* stack) {
    free(stack->data);
    free(stack);
}

int64_t execute_bytecode(uint8_t* bytecode, size_t size, Stack* stack) {
    uint8_t* pc = bytecode;
    int64_t a = 0; int64_t b = 0; int64_t c = 0;
    size_t length = size; // Use the 'size' parameter as 'length'

    while (pc < bytecode + size) {
        uint8_t opcode = *pc++;

                //printf("\nwhile pc: %ld is < bytecode: %ld + size: %ld\n", (int64_t)*(pc), (int64_t)*(bytecode), size); //decimal version of the case: opcode

        switch (opcode) {
            case 0x00: //STOP
                return stack->top; // Return the stack top and exit
            case 0x01: // ADD
                b = stack_pop(stack);
                a = stack_pop(stack);
                stack_push(stack, a + b);
                break;
            case 0x02: // MUL
                b = stack_pop(stack);
                a = stack_pop(stack);
                stack_push(stack, a * b);
                break;
            case 0x03: // SUB
                b = stack_pop(stack);
                a = stack_pop(stack);
                stack_push(stack, a - b);
                break;
            case 0x04: // DIV
                b = stack_pop(stack);
                a = stack_pop(stack);
                // Check for division by zero
                if (b == 0) {
                // Division by zero error, e.g., throw an exception or return an error code
                fprintf(stderr, "Error: Division by zero\n");
                exit(1);
                }
                stack_push(stack, a / b);
                break;
            case 0x05: // SDIV -  signed division 
                b = stack_pop(stack);
                a = stack_pop(stack);
                // Check for division by zero
                if (b == 0) {
                // Handle division by zero error, e.g., throw an exception or return an error code
                fprintf(stderr, "Error: Division by zero\n");
                exit(1);
                }
                // Perform signed division and handle overflow
                int64_t result = a / b;
                if ((a ^ b) < 0 && (a % b != 0)) {
                result = -((-(a - (a % b))) / b);
                }
                stack_push(stack, result);
                break;
            case 0x06: // MOD
                b = stack_pop(stack);
                a = stack_pop(stack);
                // Check for division by zero
                if (b == 0) {
                // Division by zero error, e.g., throw an exception or return an error code
                fprintf(stderr, "Error: Division by zero\n");
                exit(1);
                }
                stack_push(stack, a % b);
                break;
            case 0x07: // SMOD
                b = stack_pop(stack);
                a = stack_pop(stack);
                // Check for division by zero
                if (b == 0) {
                // Division by zero error, e.g., throw an exception or return an error code
                fprintf(stderr, "Error: Division by zero\n");
                exit(1);
                }
                // Perform signed modulo and handle overflow
                result = a % b;
                if ((a ^ b) < 0 && (a % b != 0)) {
                result = (-a / b) * b + a;
                }
                stack_push(stack, result);
                break;
            case 0x08: // ADDMOD
                c = stack_pop(stack);
                b = stack_pop(stack);
                a = stack_pop(stack);
                // Check for modulo by zero
                if (c == 0) {
                // Modulo by zero error, e.g., throw an exception or return an error code
                fprintf(stderr, "Error: Modulo by zero\n");
                exit(1);
                }
                // Perform addition and then modulo
                result = (a + b) % c;
                stack_push(stack, result);
                break;
            case 0x09: // MULMOD
                c = stack_pop(stack);
                b = stack_pop(stack);
                a = stack_pop(stack);
                // Check for modulo by zero
                if (c == 0) {
                // Modulo by zero error, e.g., throw an exception or return an error code
                fprintf(stderr, "Error: Modulo by zero\n");
                exit(1);
                }
                // Perform multiplication and then modulo
                result = (a * b) % c;
                stack_push(stack, result);
                break;
            case 0x0a: // EXP
                int64_t exponent = (int64_t)stack_pop(stack);
                a = (int64_t)stack_pop(stack);
                // Handle large exponents efficiently to avoid overflow
                result = 1;
                while (exponent > 0) {
                    if (exponent & 1) {
                    result *= a;
                    if (result < a) { // Overflow occurred
                    // Overflow error, e.g., throw an exception or return an error code
                    fprintf(stderr, "Error: Integer overflow in exponential operation\n");
                    exit(1);
                    }
                    }
                    a *= a;
                    if (a < 1) { // Overflow occurred
                    // Handle overflow error, e.g., throw an exception or return an error code
                    fprintf(stderr, "Error: Integer overflow in exponential operation\n");
                    exit(1);
                    }
                    exponent >>= 1;
                }
                stack_push(stack, result);
                break;
            case 0x0b: // SIGNEXTEND
                if (stack->top < 2) {
                printf("Not enough elements on the stack for SIGNEXTEND operation.\n");
                exit(1);
                }
                uint32_t b = (uint32_t)stack_pop(stack); // Cast the popped value to uint32_t
                a = stack_pop(stack);

                if (b & 0x80000000) { // If the most significant bit of b is set
                a &= ~(~0u << (b & 0x1f)); // Mask the lower 5 bits of b and apply the mask to a
                } else {
                a &= 0xffffffffffffffff; // No sign extension, keep the original value of a
                }
                stack_push(stack, a);
                break;
            case 0x10: // LT
                if (stack->top < 1) {  // stack init = -1 so 2 values is < 1
                printf("Not enough elements on the stack for LT operation.\n");
                exit(1);
                }

                b = stack_pop(stack);
                a = stack_pop(stack);

                result = (a < b) ? 1 : 0;  //int64_t result 
                stack_push(stack, result);
                break;
            case 0x11: // GT
                if (stack->top < 1) {  // stack init = -1 so 2 values is < 1
                printf("Not enough elements on the stack for LT operation.\n");
                exit(1);
                }

                b = stack_pop(stack);
                a = stack_pop(stack);

                result = (a > b) ? 1 : 0;  //int64_t result 
                stack_push(stack, result);
                break;
            case 0x12: // SLT
                if (stack->top < 1) {
                printf("Not enough elements on the stack for SLT operation.\n");
                exit(1);
                }

                b = stack_pop(stack);
                a = stack_pop(stack);

                result = (int64_t)((int64_t)a < (int64_t)b) ? 1 : 0;  //int64_t result 
                stack_push(stack, result);
                break;
            case 0x13: // SGT
                if (stack->top < 1) {
                printf("Not enough elements on the stack for SLT operation.\n");
                exit(1);
                }

                b = stack_pop(stack);
                a = stack_pop(stack);

                result = (int64_t)((int64_t)a > (int64_t)b) ? 1 : 0;  //int64_t result 
                stack_push(stack, result);
                break;
            case 0x14: // EQ
                if (stack->top < 1) {
                printf("Not enough elements on the stack for EQ operation.\n");
                exit(1);
                }

                a = stack_pop(stack);
                b = stack_pop(stack);

                result = (a == b) ? 1 : 0;  //int64_t result 
                stack_push(stack, result);
                break;
            case 0x15: // ISZERO
                if (stack->top < 0) {
                printf("Not enough elements on the stack for ISZERO operation.\n");
                exit(1);
                }

                a = stack_pop(stack);

                result = (a == 0) ? 1 : 0;  //int64_t result 
                stack_push(stack, result);
                break;
            case 0x16: // AND
                if (stack->top < 1) {
                printf("Not enough elements on the stack for AND operation.\n");
                exit(1);
                }

                a = stack_pop(stack);
                b = stack_pop(stack);

                result = a & b;  //int64_t result 
                stack_push(stack, result);
                break;
            case 0x17: // OR
                if (stack->top < 1) {
                printf("Not enough elements on the stack for OR operation.\n");
                exit(1);
                }

                a = stack_pop(stack);
                b = stack_pop(stack);

                result = a | b;  //int64_t result 
                stack_push(stack, result);
                break;
            case 0x18: // XOR
                if (stack->top < 1) {
                    printf("Not enough elements on the stack for XOR operation.\n");
                    exit(1);
                }

                a = stack_pop(stack);
                b = stack_pop(stack);

                result = a ^ b;  //int64_t result 
                stack_push(stack, result);
                break;
            case 0x19: // NOT
                if (stack->top < 0) {
                    printf("Not enough elements on the stack for NOT operation.\n");
                    exit(1);
                }

                a = stack_pop(stack);

                result = ~a;  //int64_t result 
                stack_push(stack, result);
                break;
            case 0x1a: // BYTE
                if (stack->top < 1) {
                    printf("Not enough elements on the stack for BYTE operation.\n");
                    exit(1);
                }

                b = stack_pop(stack); // byte offset
                a = stack_pop(stack); // 32-byte value

                if (b < 0 || b >= 32) {
                    printf("Byte offset out of range for BYTE operation.\n");
                    exit(1);
                }

                result = (a >> (8 * b)) & 0xFF;  //int64_t result 
                stack_push(stack, result);
                break;
            case 0x1b: // SHL
                if (stack->top < 1) {
                    printf("Not enough elements on the stack for SHL operation.\n");
                    exit(1);
                }

                b = stack_pop(stack); // shift
                a = stack_pop(stack); // value

                if (b > 255) {
                    result = 0;  // Shift greater than 255, return 0
                } else {
                    result = a << b;  //int64_t result 
                }
                stack_push(stack, result);
                break;
            case 0x1c: // SHR
                if (stack->top < 1) {
                    printf("Not enough elements on the stack for SHR operation.\n");
                    exit(1);
                }

                b = stack_pop(stack); // shift
                a = stack_pop(stack); // value

                if (b > 255) {
                    result = 0;  // Shift greater than 255, return 0
                } else {
                    result = a >> b;  //int64_t result 
                }
                stack_push(stack, result);
                break;
            case 0x1d: // SAR
                if (stack->top < 1) {
                    printf("Not enough elements on the stack for SAR operation.\n");
                    exit(1);
                }

                b = stack_pop(stack); // shift
                a = stack_pop(stack); // value

                if (b > 63) {
                    result = (a < 0) ? INT64_MIN : 0;  // Shift greater than 63, return the appropriate signed value
                } else {
                    result = a >> b;  //int64_t result 
                }
                stack_push(stack, result);
                break;
            case 0x1e:
                    //Sha32 is not used so compute Keccak-256 hash is todo Dilithium
                break;

/********************************

 **** Instructions 0x30 to 0x48 deal with blockchain operations 

*********************************/

            case 0x30:
                    //Get address of currently executing account, simply place onto the stack as part of an init
                break;
            case 0x31: // BALANCE
                if (stack->top < 0) {
                    printf("Not enough elements on the stack for BALANCE operation.\n");
                    exit(1);
                }
                //a = stack_pop(stack); // Pop the address from the stack

                // TODO function 'get_account_balance' that returns the balance in wei for a given address
                //int64_t balance = get_account_balance((char*)&a, 20); // Convert the int64_t to a char array and pass it to the function

                //stack_push(stack, balance); // Push the balance onto the stack
                break;
            case 0x32: // ORIGIN
                break;
            case 0x33: // CALLER
                break;
            case 0x34: // CALLVALUE
                break;
            case 0x35: // CALLDATALOAD
                break;
            case 0x36: // CALLDATASIZE
                break;
            case 0x37: // CODEDATACOPY
                break;
            case 0x38: // CODESIZE
                break;
            case 0x39: // CODECOPY
                break;
            case 0x3a: // GASPRICE
                break;
            case 0x3b: // EXTCODESIZE
                break;
            case 0x3c: // EXTCODESIZE
                break;
            case 0x3d: // EXTCODECOPY
                break;
            case 0x3e: // EXTCODESIZE
                break;
            case 0x3f: // EXTCODEHASH
                break;
            case 0x40: // BLOCKHASH
                break;
            case 0x41: // COINBASE
                break;
            case 0x42: // TIMESTAMP
                break;
            case 0x43: // NUMBER
                break;
            case 0x44: // PREVANDAO
                break;
            case 0x45: // GASLIMIT
                break;
            case 0x46: // CHAINID
                break;
            case 0x47: // SELFBALANCE
                break;
            case 0x48: // BASEFEE
                break;
           case 0x50: // POP
                if (stack->top < 0) {
                printf("Stack underflow in case!\n");
                exit(1);
                }
                --stack->top; // Decrement the top index without returning the value
                break;
            case 0x51: // MLOAD
                a = stack_pop(stack); // Offset
                if (a < 0 || a >= stack->memory->size * 8) {
                    printf("Error: Offset out of bounds\n");
                    return 0;
                }
                uint64_t load_word_offset = a / 8;
                uint8_t load_bit_offset = a % 8;
                uint64_t load_word = stack->memory->data[load_word_offset];
                b = (load_word >> load_bit_offset) & 0xFF;
                stack_push(stack, b);
                break;
            case 0x52: // MSTORE
                b = stack_pop(stack); // Value to store
                a = stack_pop(stack); // Offset
                if (a < 0 || a >= stack->memory->size * 8) {
                    printf("Error: Offset out of bounds\n");
                    return -1;
                }
                uint64_t store_word_offset = a / 8;
                uint8_t store_bit_offset = a % 8;
                uint64_t store_word = (stack->memory->data[store_word_offset] & ~(0xFF << store_bit_offset)) |
                                       ((b & 0xFF) << store_bit_offset);
                stack->memory->data[store_word_offset] = store_word;
                break;
            case 0x53: // MSTORE8
                b = stack_pop(stack) & 0xFF; // Value to store (truncated to 8 bits)
                a = stack_pop(stack); // Offset
                if (a < 0 || a >= stack->memory->size * 8) {
                    printf("Error: Offset out of bounds\n");
                    return -1;
                }
                uint64_t store8_byte_offset = a / 8;
                stack->memory->data[store8_byte_offset] = (stack->memory->data[store8_byte_offset] & ~(0xFF << (a % 8))) |
                                                          ((b & 0xFF) << (a % 8));
                break;
            case 0x54: // SLOAD
                a = stack_pop(stack); // Key
                int64_t** storage_address_sload = stack_get_storage_address(stack, a);
                if (*storage_address_sload == NULL) {
                stack_push(stack, 0);
                } else {
                stack_push(stack, **storage_address_sload);
                }
                break;
            case 0x55: // SSTORE
                b = stack_pop(stack); // Value
                a = stack_pop(stack); // Key
                int64_t** storage_address_sstore = stack_get_storage_address(stack, a);
                if (*storage_address_sstore == NULL) {
                *storage_address_sstore = malloc(sizeof(int64_t));
                if (*storage_address_sstore == NULL) {
                perror("Failed to allocate memory for storage value");
                exit(1);
                }
                }
                **storage_address_sstore = b;
                break;
            case 0x56: // JUMP
                a = stack_pop(stack); // Counter: byte offset in the deployed code
                if (a < 0 || a >= size) {
                printf("Invalid jump destination\n");
                return -1;
                }

                // Check if the destination is a JUMPDEST instruction
                if (bytecode[a] != 0x5b) {
                printf("Jump destination is not a JUMPDEST\n");
                return -1;
                }

                pc = bytecode + a; // Alter the program counter
                break;
            case 0x57: // JUMPI
                b = stack_pop(stack); // Condition
                a = stack_pop(stack); // Counter: byte offset in the deployed code

                if (b != 0) {
                if (a < 0 || a >= size) {
                printf("Invalid jump destination\n");
                return -1;
                }

                // Check if the destination is a JUMPDEST instruction
                if (bytecode[a] != 0x5b) {
                printf("Jump destination is not a JUMPDEST\n");
                return -1;
                }

                pc = bytecode + a; // Alter the program counter if the condition is true
                }
                break;
            case 0x58: // PC
                stack_push(stack, (int64_t)(pc - bytecode)); // Push the program counter value onto the stack
                break;
            case 0x59: // MSIZE
                stack_push(stack, (int64_t)(stack->memory->size * 8)); // Push the memory size in bytes (times 8 to convert from words to bytes)
                break;
            case 0x5a:
                    //GAS
                break;
            case 0x5b:
                    //JUMPDEST - check for JUMPDEST when performing a JUMP or JUMPI operation to validate destination.
                break;
            case 0x5f: // PUSH0
                stack_push(stack, 0); // Push the value 0 onto the stack
                break;
            case 0x60: // PUSH1 - single byte, stack_push(stack, (int64_t)*(pc++));
            case 0x61: // PUSH2
            case 0x62: // PUSH3
            case 0x63: // PUSH4
            case 0x64: // PUSH5
            case 0x65: // PUSH6
            case 0x66: // PUSH7
            case 0x67: // PUSH8
            case 0x68: // PUSH9
            case 0x69: // PUSH10
            case 0x6a: // PUSH11
            case 0x6b: // PUSH12
            case 0x6c: // PUSH13
            case 0x6d: // PUSH14
            case 0x6e: // PUSH15
            case 0x6f: // PUSH16
            case 0x70: // PUSH17
            case 0x71: // PUSH18
            case 0x72: // PUSH19
            case 0x73: // PUSH20
            case 0x74: // PUSH21
            case 0x75: // PUSH22
            case 0x76: // PUSH23
            case 0x77: // PUSH24
            case 0x78: // PUSH25
            case 0x79: // PUSH26
            case 0x7a: // PUSH27
            case 0x7b: // PUSH28
            case 0x7c: // PUSH29
            case 0x7d: // PUSH30
            case 0x7e: // PUSH31
            case 0x7f: // PUSH32

                uint8_t n = opcode - 95; //0x60 = 96 ~ 0x7f = 127
                //printf("Which push? %d\n", opcode - 95); //debug
                
                for (int i = 0; i < n; i++) {
                //printf("\nPC: %ld\n", (int64_t)*(pc++)); //pc debug
                stack_push(stack, (int64_t)*(pc++));
                }

                break;
            case 0x80: // DUP1
            case 0x81:
            case 0x82:
            case 0x83:
            case 0x84:
            case 0x85:
            case 0x86:
            case 0x87:
            case 0x88:
            case 0x89:
            case 0x8A:
            case 0x8B:
            case 0x8C:
            case 0x8D:
            case 0x8E:
            case 0x8F:
                stack_dup(stack, opcode - 127);
            break;
            case 0x90: // SWAP1
            case 0x91:
            case 0x92:
            case 0x93:
            case 0x94:
            case 0x95:
            case 0x96:
            case 0x97:
            case 0x98:
            case 0x99:
            case 0x9A:
            case 0x9B:
            case 0x9C:
            case 0x9D:
            case 0x9E:
            case 0x9F:
                stack_swap(stack, opcode - 143); //0x90 = 144);
            break;
            case 0xa0: // LOG 0 - 4
            case 0xa1:
            case 0xa2:
            case 0xa3:
            case 0xa4:
                stack_LOG(stack, opcode - 160); //0a = 160
            break;
            case 0xf0: //Create new contract        
            break;
            case 0xf1: //code       
            break;
            case 0xf2: //callcode        
            break;
            case 0xf3: //return       
            break;
            case 0xf4: //DELEGATECALL
            break;
            case 0xf5: //CREATE2
            break;
            case 0xfa: //STATICCALL
            break;
            case 0xfd: //REVERT
            break;
            case 0xfe: //INVALID
            break;
            case 0xff: //SELFDESTRUCT
            break;
            default:
                printf("Invalid opcode: 0x%02X\n", opcode);
                exit(1);
        }

                print_stack(stack); //debug

    }

    return stack_pop(stack);
}

int main() {

    //Start the stack instance
    Stack* stack = stack_create(1024);

    //Execute code
    uint8_t bytecode1[] = {0x60, 0x01, 0x60, 0x02, 0x01}; //ADD
    printf("Result ADD: %ld\n\n", execute_bytecode(bytecode1, sizeof(bytecode1), stack));

    //print_stack(stack) executed in execute_bytecode function

    //Destroy the stack
    stack_destroy(stack); 

    //Test every isntruction
    Stack* stack2 = stack_create(1024);

    uint8_t bytecode_stop[] = {0x60, 0x01, 0x60, 0x02, 0x00}; //STOP
    printf("Result STOP: %ld\n\n", execute_bytecode(bytecode_stop, sizeof(bytecode_stop), stack2));

    uint8_t bytecode2[] = {0x60, 0x03, 0x60, 0x02, 0x02}; //MUL
    printf("Result MUL: %ld\n\n", execute_bytecode(bytecode2, sizeof(bytecode2), stack2));

    uint8_t bytecode3[] = {0x60, 0x03, 0x60, 0x04, 0x03}; //SUB
    printf("Result SUB: %ld\n\n", execute_bytecode(bytecode3, sizeof(bytecode3), stack2));

    uint8_t bytecode4[] = {0x60, 0x24, 0x60, 0x12, 0x04}; //DIV
    printf("Result DIV: %ld\n\n", execute_bytecode(bytecode4, sizeof(bytecode4), stack2));

    uint8_t bytecode5[] = {0x60, 0x24, 0x60, 0x12, 0x05}; //SDIV
    printf("Result SDIV: %ld\n\n", execute_bytecode(bytecode5, sizeof(bytecode5), stack2));

    uint8_t bytecode6[] = {0x60, 0x26, 0x60, 0x12, 0x06}; //MOD
    printf("Result MOD: %ld\n\n", execute_bytecode(bytecode6, sizeof(bytecode6), stack2));

    uint8_t bytecode7[] = {0x60, 0x28, 0x60, 0x12, 0x07}; //SMOD
    printf("Result SMOD: %ld\n\n", execute_bytecode(bytecode7, sizeof(bytecode7), stack2));

    uint8_t bytecode8[] = {0x60, 0x28, 0x60, 0x13, 0x60, 0x05, 0x08}; //ADDMOD
    printf("Result ADDMOD: %ld\n\n", execute_bytecode(bytecode8, sizeof(bytecode8), stack2));

    uint8_t bytecode9[] = {0x60, 0x28, 0x60, 0x13, 0x60, 0x05, 0x09}; //MULMOD
    printf("Result MULMOD: %ld\n\n", execute_bytecode(bytecode9, sizeof(bytecode9), stack2));

    uint8_t bytecode10[] = {0x60, 0x02, 0x60, 0x03, 0x0a}; //EXP
    printf("Result EXP: %ld\n\n", execute_bytecode(bytecode10, sizeof(bytecode10), stack2));

    uint8_t bytecode11[] = {0x60, 0x01, 0x60, 0x01, 0x60, 0x80, 0x0b}; // PUSH1 1, PUSH1 -128, SIGNEXTEND
    printf("Result SIGNEXTEND: %ld\n\n", execute_bytecode(bytecode11, sizeof(bytecode11), stack2));

    stack_clear(stack2);

    uint8_t bytecode12[] = {0x60, 0x09, 0x60, 0x0a, 0x10}; //LT
    printf("Result LT: %ld\n\n", execute_bytecode(bytecode12, sizeof(bytecode12), stack2));

    uint8_t bytecode13[] = {0x60, 0x09, 0x60, 0x0a, 0x11}; //GT
    printf("Result GT: %ld\n\n", execute_bytecode(bytecode13, sizeof(bytecode13), stack2));

    uint8_t bytecode14[] = {0x60, 0x09, 0x60, 0x0a, 0x12}; //SLT
    printf("Result SLT: %ld\n\n", execute_bytecode(bytecode14, sizeof(bytecode14), stack2));

    uint8_t bytecode15[] = {0x60, 0x09, 0x60, 0x0a, 0x13}; //SGT
    printf("Result SLT: %ld\n\n", execute_bytecode(bytecode15, sizeof(bytecode15), stack2));

    stack_clear(stack2);

    uint8_t bytecode16[] = {0x60, 0x0a, 0x60, 0x0a, 0x14}; //EQ
    printf("Result EQ: %ld\n\n", execute_bytecode(bytecode16, sizeof(bytecode16), stack2));

    uint8_t bytecode17[] = {0x60, 0x00, 0x15}; //ISZERO
    printf("Result ISZERO: %ld\n\n", execute_bytecode(bytecode17, sizeof(bytecode17), stack2));

    uint8_t bytecode18[] = {0x60, 0x0F, 0x60, 0xFF, 0x16}; // PUSH1 15, PUSH1 255, AND
    printf("Result AND: %ld\n\n", execute_bytecode(bytecode18, sizeof(bytecode18), stack2));

    stack_clear(stack2);

    uint8_t bytecode19[] = {0x60, 0x0F, 0x60, 0x0A, 0x17}; // PUSH1 15, PUSH1 10, OR
    printf("Result OR: %ld\n\n", execute_bytecode(bytecode19, sizeof(bytecode19), stack2));

    uint8_t bytecode_xor[] = {0x60, 0x0F, 0x60, 0x0A, 0x18}; // PUSH1 15, PUSH1 10, XOR
    printf("Result XOR: %ld\n\n", execute_bytecode(bytecode_xor, sizeof(bytecode_xor), stack2));

    uint8_t bytecode_not[] = {0x60, 0x0A, 0x19}; // PUSH1 10, NOT
    printf("Result NOT: %ld\n\n", execute_bytecode(bytecode_not, sizeof(bytecode_not), stack2));

    uint8_t bytecode_byte[] = {0x60, 0x20, 0x60, 0x01, 0x1a}; // PUSH1 32, PUSH1 1, BYTE
    printf("Result BYTE: %ld\n\n", execute_bytecode(bytecode_byte, sizeof(bytecode_byte), stack2));

    uint8_t bytecode_shl[] = {0x60, 0x01, 0x60, 0x02, 0x1b}; // PUSH1 1, PUSH1 2, SHL
    printf("Result SHL: %ld\n\n", execute_bytecode(bytecode_shl, sizeof(bytecode_shl), stack2));

    uint8_t bytecode_shr[] = {0x60, 0x01, 0x60, 0x02, 0x1c}; // PUSH1 1, PUSH1 2, SHR
    printf("Result SHR: %ld\n\n", execute_bytecode(bytecode_shr, sizeof(bytecode_shr), stack2));

    uint8_t bytecode_sar[] = {0x60, 0x01, 0x60, 0x02, 0x1d}; // PUSH1 1, PUSH1 2, SAR
    printf("Result SAR: %ld\n\n", execute_bytecode(bytecode_sar, sizeof(bytecode_sar), stack2));

    // Test MSTORE

    stack_clear(stack2);

    uint8_t bytecode_push0[] = {
 0x60, 0x0a,
 0x61, 0x08, 0x07, 
 0x62, 0x07, 0x06, 0x05, 
 0x63, 0x04, 0x03, 0x02, 0x01, 
 0x64, 0x09, 0x08, 0x07, 0x06, 0x05,
 0x65, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08,
 0x66, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,
 0x67, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02,
 0x68, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,
 0x69, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09,
 0x6a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08,
 0x6b, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07,
 0x6c, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06,
 0x6d, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x05,
 0x6e, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04,
 0x6f, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03,
 0x70, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02,
 0x71, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,
 0x72, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09,
 0x73, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08,
 0x74, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07,
 0x75, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06,
 0x76, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x05,
 0x77, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x08, 0x07, 0x06, 0x05,
 0x78, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x08, 0x07, 0x06, 0x05,
 0x79, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x08, 0x07, 0x06, 0x05,
 0x7a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x08, 0x07, 0x06, 0x05, 0x04,
 0x7b, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x08, 0x07, 0x06, 0x05, 0x04,  0x03,
 0x7c, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02,
 0x7d, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x08, 0x07, 0x06, 0x05, 0x04,  0x03, 0x02, 0x01,
 0x7e, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09,
 0x7f, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x08, 0x07, 0x06, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x09, 0x0a
};

    printf("Result: %ld\n", execute_bytecode(bytecode_push0, sizeof(bytecode_push0), stack2));

   stack_clear(stack2);

//Test mload 0x51, mstore 0x52, mstore8 0x53
uint8_t bytecode_mstore[] = {0x60, 0x01, 0x62, 0x02, 0x03, 0x04, 0x52}; // MSTORE
execute_bytecode(bytecode_mstore, sizeof(bytecode_mstore), stack2);

uint8_t bytecode_mload[] = {0x60, 0x05, 0x51, 0x60, 0x06, 0x50}; // MLOAD
execute_bytecode(bytecode_mload, sizeof(bytecode_mload), stack2);

   stack_clear(stack2);

uint8_t bytecode_mstore8[] = {0x60, 0x07, 0x60, 0x08, 0x53, 0x60, 0x09, 0x51, 0x60, 0x0a, 0x50}; // MSTORE8, MLOAD
execute_bytecode(bytecode_mstore8, sizeof(bytecode_mstore8), stack2);

   stack_clear(stack2);

uint8_t bytecode_sstore[] = {0x61, 0x0b, 0x02, 0x60, 0x03, 0x55}; // SSTORE
execute_bytecode(bytecode_sstore, sizeof(bytecode_sstore), stack2);

uint8_t bytecode_sload[] = {0x60, 0x00, 0x54}; // SLOAD
execute_bytecode(bytecode_sload, sizeof(bytecode_sload), stack2);

//JUMP - Test bytecode for JUMP


//PC 0x58
uint8_t bytecode_pc[] = {0x58}; // PC
// Execute the bytecode and print the result
int64_t result = execute_bytecode(bytecode_pc, sizeof(bytecode_pc), stack2);

// MSIZE 0x59
uint8_t bytecode_msize[] = {0x59}; // MSIZE
result = execute_bytecode(bytecode_msize, sizeof(bytecode_msize), stack2);

   stack_clear(stack2);

//DUP

    uint8_t bytecode_d[] = {0x65, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x83};
    execute_bytecode(bytecode_d, sizeof(bytecode_d), stack2);

//Swaps

   stack_clear(stack2);

    uint8_t bytecode_swap[] = {0x63, 0x04, 0x03, 0x02, 0x01, 0x92};
    execute_bytecode(bytecode_swap, sizeof(bytecode_swap), stack2);

   stack_clear(stack2);

//Logs - 0 .. 4
    uint8_t bytecode_LOG0[] = {0x61, 0x01, 0x02, 0xa0, 0x60, 0x00};
    execute_bytecode(bytecode_LOG0, sizeof(bytecode_LOG0), stack2);
    uint8_t bytecode_LOG1[] = {0x62, 0x01, 0x02, 0x03, 0xa1, 0x60, 0x00};
    execute_bytecode(bytecode_LOG1, sizeof(bytecode_LOG1), stack2);
    uint8_t bytecode_LOG2[] = {0x63, 0x01, 0x02, 0x03, 0x04, 0xa2, 0x60, 0x00};
    execute_bytecode(bytecode_LOG2, sizeof(bytecode_LOG2), stack2);
    uint8_t bytecode_LOG3[] = {0x64, 0x01, 0x02, 0x03, 0x04, 0x05, 0xa3, 0x60, 0x00};
    execute_bytecode(bytecode_LOG3, sizeof(bytecode_LOG3), stack2);
    uint8_t bytecode_LOG4[] = {0x65, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0xa4, 0x60, 0x00};
    execute_bytecode(bytecode_LOG4, sizeof(bytecode_LOG4), stack2);

   return 0;
}
