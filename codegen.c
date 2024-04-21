#include "9cc.h"

static int label_count = 0;
char *argreg1[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"};
char *argreg2[] = {"di", "si", "dx", "cx", "r8w", "r9w"};
char *argreg4[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
char *argreg8[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

char *funcname;

void gen_addr(Node *node) {
    switch (node->kind)
    {
        case ND_VAR: {
            Var *var = node->var;
            if (var->is_local) {
                printf("  lea rax, [rbp-%d]\n", var->offset);
                printf("  push rax\n");
            }
            else {
                printf("  push offset %s\n", var->name);
            }
            return;
        }
        case ND_DEREF: {
            gen(node->lhs);
            return;
        }
        case ND_MEMBER: {
            gen_addr(node->lhs);
            printf("  pop rax\n");
            printf("  add rax, %d\n", node->member->offset);
            printf("  push rax\n");
            return;
        }
    }
    error_tok(node->tok, "not an local value\n");
    // if (node->kind != ND_VAR) {
    //     error("Substitution of the left value does not a variable. actual: %d", node->kind);
    // }

}

void gen_lval(Node *node) {
    if (node->ty->kind == TY_ARRAY) {
        error_tok(node->tok, "not an local value");
    }
    gen_addr(node);
}
void load(Type *ty) {
    printf("  pop rax\n");

    int sz = size_of(ty);
    if (sz == 1) {
        printf("  movsx rax, byte ptr [rax]\n");
    } else if (sz == 2) {
        printf("  movsx rax, word ptr [rax]\n");
    } else if (sz == 4) {
        printf("  movsxd rax, dword ptr [rax]\n");
    } else {
        assert(sz == 8);
        printf("  mov rax, [rax]\n");
    }
    printf("  push rax\n");
}
void store(Type *ty) {
    printf("  pop rdi\n");
    printf("  pop rax\n");

    if (ty->kind == TY_BOOL) {
        printf("  cmp rdi, 0\n");
        printf("setne dil\n");
        printf("  movzb rdi, dil\n");
    }
    int sz = size_of(ty);
    if (sz == 1) {
        printf("  mov [rax], dil\n");
    } else if (sz == 2) {
        printf("  mov [rax], di\n");
    } else if (sz == 4) {
        printf("  mov [rax], edi\n");
    }
    else {
        assert(sz == 8);
        printf("  mov [rax], rdi\n");
    }
    printf("  push rdi\n");
}

void gen(Node *node) {
    
    if (!node) return;


    switch (node->kind) {
    case ND_NULL:
        return;
    case ND_NUM:
        printf("  push %d\n", node->val);
        return;
    case ND_EXPR_STMT:
        gen(node->lhs);
        printf("  add rsp, 8\n");
        return;
    case ND_VAR:
    case ND_MEMBER:
        gen_addr(node);
        if (node->ty->kind != TY_ARRAY) {
            load(node->ty);
        }
        return;
    case ND_ASSIGN:
        gen_lval(node->lhs);
        gen(node->rhs);

        store(node->ty);
        return;
    case ND_ADDR: {
        gen_addr(node->lhs);
        return;
    }
    case ND_DEREF: {
        gen(node->lhs);
       if (node->ty->kind != TY_ARRAY) {
           load(node->ty);
       }
        return;
    }
    case ND_IF: {
        int cnt = label_count++;
        if (node->els) {
            gen(node->cond);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je  .Lelse%d\n", cnt);
            gen(node->then);
            printf("  jmp .Lend%d\n", cnt);
            printf(".Lelse%d:\n", cnt);
            gen(node->els);
            printf(".Lend%d:\n", cnt);
        } else {
            gen(node->cond);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je  .Lend%d\n", cnt);
            gen(node->then);
            printf(".Lend%d:\n", cnt);
        }
        return;
    }
    case ND_WHILE: {
        int cnt = label_count++;
        printf(".Lbegin%d:\n", cnt);
        gen(node->cond);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  je  .Lend%d\n", cnt);
        gen(node->then);
        printf("  jmp .Lbegin%d\n", cnt);
        printf(".Lend%d:\n", cnt);
        return;
    }
    case ND_FOR: {
        int cnt = label_count++;
        if (node->init) {
            gen(node->init);
        }
        printf(".Lbegin%d:\n", cnt);
        if (node->cond) {
            gen(node->cond);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je  .Lend%d\n", cnt);
        }
        gen(node->then);
        if (node->inc) {
            gen(node->inc);
        }
        printf("  jmp .Lbegin%d\n", cnt);
        printf(".Lend%d:\n", cnt);
        return;
    }
    case ND_BLOCK:
        for (Node *n = node->body; n; n = n->next) {
            gen(n);
        }
        return;
    case ND_STMT_EXPR:
        for (Node *n = node->body; n; n = n->next) {
            gen(n);
        }
        return;
    case ND_FUNCALL: {
        int nargs = 0;
        for (Node *arg = node->args; arg; arg = arg->next) {
            gen(arg);
            nargs++;
        }
        for (int i = nargs - 1; i >= 0; i--) {
            printf("  pop %s\n", argreg8[i]);
        }

        // We need to align RSP to a 16 byte boundary before
        // calling a function because it is an ABI requirement.
        // RAX is set to 0 for variadic function.
        int cnt = label_count++;
        printf("  mov rax, rsp\n");
        printf("  and rax, 15\n");
        printf("  jnz .Lcall%d\n", cnt);
        printf("  mov rax, 0\n");
        printf("  call %s\n", node->funcname);
        printf("  jmp .Lend%d\n", cnt);
        printf(".Lcall%d:\n", cnt);
        printf("  sub rsp, 8\n");
        printf("  mov rax, 0\n");
        printf("  call %s\n", node->funcname);
        printf("  add rsp, 8\n");
        printf(".Lend%d:\n",cnt);
        printf("  push rax\n");
        return;
    }
    case ND_RETURN:
        gen(node->lhs);
        printf("  pop rax\n");
        printf("  jmp .Lreturn.%s\n", funcname);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->kind) {
        case ND_ADD:
            if (node->ty->base) {
                
                printf("  imul rdi, %d\n", size_of(node->ty->base));
            }
            printf("  add rax, rdi\n");
            break;
        case ND_SUB:
        if (node->ty->base) {
                printf("  imul rdi, %d\n", size_of(node->ty->base));
            }
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

void emit_data(Program *prog) {
    printf(".data\n");

    for (VarList *vl = prog->globals; vl; vl = vl->next) {
        Var *var = vl->var;
        printf("%s:\n", var->name);
        if (!var->contents) {
            printf("  .zero %d\n", size_of(var->ty));
        }
        
        for (int i = 0; i < var->cont_len; i++) {
            printf("  .byte %d\n", var->contents[i]);
        }
        
    }
}

void load_arg(Var *var, int idx) {
    int sz = size_of(var->ty);
    if (sz == 1) {
        printf("  mov [rbp-%d], %s\n", var->offset, argreg1[idx]);
    } else if (sz == 2) {
        printf("  mov [rbp-%d], %s\n", var->offset, argreg2[idx]);
    } else if (sz == 4) {
        printf("  mov [rbp-%d], %s\n", var->offset, argreg4[idx]);
    }
    else {
        assert(sz == 8);
        printf("  mov [rbp-%d], %s\n", var->offset, argreg8[idx]);
    }
}

void emit_text(Program *prog) {
    printf(".text\n");
    for (Function *fn = prog->fns; fn; fn = fn->next) {
        printf(".global %s\n", fn->name);
        printf("%s:\n", fn->name);
        funcname = fn->name;

        // prologue
        printf("  push rbp\n");
        printf("  mov rbp, rsp\n");
        printf("  sub rsp, %d\n", fn->stack_size);

        int i = 0;
        for (VarList *vl = fn->params; vl; vl = vl->next) {
            load_arg(vl->var, i++);
        }

        //emit code
        for (Node *node = fn->node; node; node = node->next) {
            gen(node);
        }

        // epilogue
        printf(".Lreturn.%s:\n", funcname);
        printf("  mov rsp, rbp\n");
        printf("  pop rbp\n");
        printf("  ret\n");

    }

}

void codegen(Program *prog) {
    printf(".intel_syntax noprefix\n");

    emit_data(prog);
    emit_text(prog);
}