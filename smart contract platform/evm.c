#include <stdio.h>
#include <stdlib.h>

#define MAX_STACK_SIZE 1024

typedef struct evm_state {
    int stack[MAX_STACK_SIZE];
    int stack_pointer;
} evm_state;

void execute_contract(evm_state* state, const char* bytecode) {
    int pc = 0;
    int opcode;
    int a, b;

    while (bytecode[pc] != 0x00) {
        opcode = bytecode[pc++];
        printf("Current opcode: %d\n", opcode);
        switch (opcode) {
            case 0x01: // ADD
                a = state->stack[--state->stack_pointer];
                b = state->stack[--state->stack_pointer];
                if (state->stack_pointer >= 0 && state->stack_pointer < MAX_STACK_SIZE)
                state->stack[state->stack_pointer++] = a + b;
                break;
            default:
                printf("Unknown opcode: %d\n", opcode);
                exit(1);
        }
    }
}

int main() {
    evm_state state;
    state.stack_pointer = 0;

    // Load the bytecode from the file "smart_contract.bin"
    FILE* fp = fopen("smart_contract.bin", "rb");
    if (!fp) {
        printf("Failed to open file 'smart_contract.bin'\n");
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    rewind(fp);
    char* bytecode = (char*)malloc(sizeof(char) * size);
    fread(bytecode, 1, size, fp);
    fclose(fp);

    // Execute the contract
    execute_contract(&state, bytecode);

    // Output the result
    printf("Result: %d\n", state.stack[0]);

    free(bytecode);
    return 0;
}

