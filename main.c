#include "9cc.h"

Node *code[100];

int main(int argc, char **argv) {

    if (argc != 2) {
        fprintf(stderr, "invalid args\n");
        return 1;
    }
    locals = NULL;
    // tokenize and parse input
    user_input = argv[1];
    token = tokenize(user_input);
    program();


    // the template of head of our assembly
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    // prologue
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, 208\n");
    
    for (int i = 0; code[i]; i++) {
        // generates codes 
        gen(code[i]);
        printf("  pop rax\n");
    }

    // epilogue
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return 0;
}
