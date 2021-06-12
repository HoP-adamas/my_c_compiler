#include "9cc.h"

static int label_count = 0;

void gen_lval(Node *node) {
    if (node->kind != ND_LVAR) {
        error("Substitution of the left value does not a variable");
    }

    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->offset);
    printf("  push rax\n");
}

void gen(Node *node) {
    
    if (!node) return;

    int cnt = label_count;
    label_count++;

    switch (node->kind) {
    case ND_NUM:
        printf("  push %d\n", node->val);
        return;
    case ND_LVAR:
        gen_lval(node);
        printf("  pop rax\n");
        printf("  mov rax, [rax]\n");
        printf("  push rax\n");
        return;
    case ND_ASSIGN:
        gen_lval(node->lhs);
        gen(node->rhs);

        printf("  pop rdi\n");
        printf("  pop rax\n");
        printf("  mov [rax], rdi\n");
        printf("  push rdi\n");
        return;
    case ND_IF:
        gen(node->cond);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  je .Lelse%d\n", cnt);
        gen(node->then);
        printf("  jmp .Lend%d\n", cnt);
        printf(".Lelse%d:\n", cnt);
        if (node->els) {
            gen(node->els);
        }
        printf(".Lend%d:\n", cnt);
        return;
    case ND_WHILE:
        printf(".Lbegin%d:\n", cnt);
        gen(node->cond);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  je .Lend%d\n", cnt);
        gen(node->body);
        printf("  jmp .Lbegin%d\n", cnt);
        printf(".Lend%d:\n", cnt);
        return;
    case ND_FOR:
        gen(node->init);
        printf(".Lbegin%d:\n", cnt);
        gen(node->cond);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  je .Lend%d\n", cnt);
        gen(node->body);
        gen(node->inc);
        printf("  jmp .Lbegin%d\n", cnt);
        printf(".Lend%d:\n", cnt);
        return;
    case ND_BLOCK:
        for (Node *n = node->body; n; n = n->next) {
            gen(n);
        }
        return;
    case ND_RETURN:
        gen(node->lhs);
        printf("  pop rax\n");
        printf("  mov rsp, rbp\n");
        printf("  pop rbp\n");
        printf("  ret\n");
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