#include "9cc.h"

Type *int_type(void) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_INT;
    return ty;
}

Type *pointer_to(Type *base) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_PTR;
    ty->base = base;
    return ty;
}
int size_of(Type *ty) {
    switch (ty->kind) {
        case TY_INT:
            return 4;
        case TY_PTR:
            return 8;
    }
    return size_of(ty->base);
}

void visit(Node *node) {
    if (!node) {
        return;
    }

    visit(node->lhs);
    visit(node->rhs);
    visit(node->cond);
    visit(node->then);
    visit(node->els);
    visit(node->init);
    visit(node->inc);

    for (Node *n = node->body; n; n = n->next) {
        visit(n);
    }
    for (Node *n = node->args; n; n = n->next) {
        visit(n);
    }

    switch(node->kind) {
        case ND_MUL:
        case ND_DIV:
        case ND_EQ:
        case ND_NE:
        case ND_LT:
        case ND_LE:
        case ND_FUNCALL:
        case ND_NUM:
            node->ty = int_type();
            return;
        case ND_VAR:
            node->ty = node->var->ty;
            return;
        case ND_ADD:
            if (node->rhs->ty->kind == TY_PTR) {
                Node *tmp = node->lhs;
                node->lhs = node->rhs;
                node->rhs = tmp;
            }
            if (node->rhs->ty->kind == TY_PTR) {
                error("invalid pointer arithmetic operands");
            }
            node->ty = node->lhs->ty;
            return;
        case ND_SUB:
            if (node->rhs->ty->kind == TY_PTR) {
                Node *tmp = node->lhs;
                node->lhs = node->rhs;
                node->rhs = tmp;
            }
            if (node->rhs->ty->kind == TY_PTR) {
                error("invalid pointer arithmetic operands");
            }
            node->ty = node->lhs->ty;
            return;
        case ND_ASSIGN:
            node->ty = node->lhs->ty;
            return;
        case ND_ADDR:
            node->ty = pointer_to(node->lhs->ty);
            return;
        case ND_DEREF:
            if (node->lhs->ty->kind != TY_PTR) {
                node->ty = node->lhs->ty->base;
            } else {
                node->ty = int_type();
            }
            return;
        case ND_SIZEOF:
            node->kind = ND_NUM;
            node->ty = int_type();
            node->val = size_of(node->lhs->ty);
            node->lhs = NULL;
            return;


        
    }
}

void add_type(Function *prog) {
    for (Function *fn = prog; fn; fn = fn->next) {
        for (Node *node = fn->node; node; node = node->next) {
            visit(node);
        }
    }
}