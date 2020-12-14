#include "9cc.h"

// the token we focus on
Token *token;

char *user_input;

// Reports an error location and exit.
void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// creates a new node
Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

// creates a new node whose kind is ND_NUM
Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

Node *expr(void);
Node *mul(void);
Node *primary(void);


// creates expr := mul ("+" mul|"-" mul)*
Node *expr(void) {
    Node *node = mul();

    for (; ; ) {
        if (consume('+')){
            node = new_node(ND_ADD, node, mul());
        }
        else if (consume('-')) {
            node = new_node(ND_SUB, node, mul());
        }
        else {
            return node;
        }
    }
}

// creates mul := primary ("*" primary|"/" primary)*
Node *mul(void) {
    Node *node = primary();
    for (; ; ) {
        if (consume('*')){
            node = new_node(ND_MUL, node, primary());
        }
        else if (consume('/')) {
            node = new_node(ND_DIV, node, primary());
        }
        else {
            return node;
        }
    }
}

// creates primary := num | "(" expr ")"
Node *primary(void) {
    if (consume('(')) {
        Node *node = expr();
        expect(')');
        return node;
    }
    return new_node_num(expect_number());
}

void gen(Node *node) {
    if (node->kind == ND_NUM) {
        printf("  push %d\n", node->val);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->kind) {
        case ND_ADD:
            printf("  add rax, rdi\n");
            break;
        case ND_SUB:
            printf("  sub rax, rdi\n");
            break;
        case ND_MUL:
            printf("  imul rax, rdi\n");
            break;
        case ND_DIV:
            printf("  cqo\n");
            printf("  idiv rdi\n");
            break;
    }
    printf("  push rax\n");
}

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
