#include "9cc.h"

// the token we focus on
Token *token;

int main(int argc, char **argv) {

    if (argc != 2) {
        fprintf(stderr, "invalid args\n");
        return 1;
    }

    token = tokenize(argv[1]);

    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    // The first token must be a number
    printf("  mov rax, %d\n", expect_number());
    
    while (!at_eof()) {
        if (consume('+')) {
            printf("  add rax, %d\n", expect_number());
            continue;
        }

        consume('-');
        printf("  sub rax, %d\n", expect_number());
    }
    printf("  ret\n");
        return 0;
}
