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
Node *equality(void);
Node *relational(void);
Node *add(void);
Node *mul(void);
Node *unary(void);
Node *primary(void);


// creates expr := equality
Node *expr(void) {
    return equality();
}

// creates equality := relational ("==" relational | "!=" relational)*
Node *equality(void) {
    Node *node = relational();
    for (; ; ) {
        if (consume("==")) {
            node = new_node(ND_EQ, node, relational());
        }
        else if (consume("!=")) {
            node = new_node(ND_NE, node, relational());
        }
        else {
            return node;
        }
    }
}

// creates relational := add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational(void) {
    Node *node = add();
    for (; ; ) {
        if (consume("<")) {
            node = new_node(ND_LT, node, add());
        }
        else if (consume("<=")) {
            node = new_node(ND_LE, node, add());
        }
        else if (consume(">")) {
            node = new_node(ND_LT, add(), node);
        }
        else if (consume(">=")) {
            node = new_node(ND_LE, add(), node);
        }
        else {
            return node;
        }
    }
}

// creates add := mul ("+" mul | "-" mul)*
Node *add(void) {
    Node *node = mul();

    for (; ; ) {
        if (consume("+")){
            node = new_node(ND_ADD, node, mul());
        }
        else if (consume("-")) {
            node = new_node(ND_SUB, node, mul());
        }
        else {
            return node;
        }
    }
}

// creates mul := unary ("*" unary|"/" unary)*
Node *mul(void) {
    Node *node = unary();
    for (; ; ) {
        if (consume("*")){
            node = new_node(ND_MUL, node, unary());
        }
        else if (consume("/")) {
            node = new_node(ND_DIV, node, unary());
        }
        else {
            return node;
        }
    }
}

// creates unary := ('+'|'-')? primary
Node *unary(void) {
    if (consume("+")) {
        return unary();
    }
    if (consume("-")) {
        return new_node(ND_SUB, new_node_num(0), unary());
    }
    return primary();
}

// creates primary := num | "(" expr ")"
Node *primary(void) {
    if (consume("(")) {
        Node *node = expr();
        expect(")");
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
        case ND_EQ:
            printf("  cmp rax, rdi\n");
            printf("  sete al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_NE:
            printf("  cmp rax, rdi\n");
            printf("  setne al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_LT:
            printf("  cmp rax, rdi\n");
            printf("  setl al\n");
            printf("  movzb rax, al\n");
            break;

        case ND_LE:
            printf("  cmp rax, rdi\n");
            printf("  setle al\n");
            printf("  movzb rax, al\n");
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
