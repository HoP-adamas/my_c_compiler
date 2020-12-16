#include "9cc.h"

// the token we focus on
Token *token;

char *user_input;

int main(int argc, char **argv) {

    if (argc != 2) {
        fprintf(stderr, "invalid args\n");
        return 1;
    }

    // tokenize and parse input
    user_input = argv[1];
    token = tokenize(user_input);
    Node *node = expr();


    // the template of head of our assembly
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");
    
    // generates codes 
    gen(node);

    printf("  pop rax\n");
    printf("  ret\n");
    return 0;
}
