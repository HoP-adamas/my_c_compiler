#include "9cc.h"

// Scope for local variables, global variables or typedef
typedef struct VarScope VarScope;
struct VarScope {
    VarScope *next;
    char *name;
    Var *var;
    Type *type_def;
};

// Scope for struct tags
typedef struct TagScope TagScope;
struct TagScope {
    TagScope *next;
    char *name;
    Type *ty;
};
VarList *locals;
VarList *globals;

VarScope *var_scope;
TagScope *tag_scope;

// Find a variable or a typedef by name.
VarScope *find_Var(Token *tok) {

    // find a variable by its name.
    for (VarScope *sc = var_scope; sc; sc = sc->next) {
        if (strlen(sc->name) == tok->len && !memcmp(tok->str, sc->name, tok->len)) {
            return sc;
        }
    }
    return NULL;
}

TagScope *find_tag(Token *tok) {
    for(TagScope *sc = tag_scope; sc; sc = sc->next) {
        if (strlen(sc->name) == tok->len && !memcmp(tok->str, sc->name, tok->len)) {
            return sc;
        }
    }
    return NULL;
}

Type *find_typedef(Token *tok) {
    if (tok->kind == TK_IDENT) {
        VarScope *sc = find_Var(token);
        if (sc) {
            return sc->type_def;
        }
    }
    return NULL;
}

Node *new_node(NodeKind kind, Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->tok = tok;
    return node;
}

// creates a new node
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_unary(NodeKind kind, Node *expr, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = expr;
    return node;
}

// creates a new node whose kind is ND_NUM
Node *new_node_num(int val, Token *tok) {
    Node *node = new_node(ND_NUM, tok);
    node->val = val;
    return node;
}

Node *new_node_Var(Var *var, Token *tok) {
    Node *node = new_node(ND_VAR, tok);
    node->var = var;
    return node;
}

VarScope *push_scope(char *name) {
    VarScope *sc = calloc(1, sizeof(VarScope));
    sc->name = name;
    sc->next = var_scope;
    var_scope = sc;
    return sc;

}

Var *push_var(Type *ty, char *name, bool is_local) {
    Var *var = calloc(1, sizeof(Var));
    var->name = name;
    var->ty = ty;
    var->is_local = is_local;
    VarList *vl = calloc(1, sizeof(VarList));
    vl->var = var;
    if (is_local) {
        vl->next = locals;
        locals = vl;
    }
    else if (ty->kind != TY_FUNC) {
        vl->next = globals;
        globals = vl;
    }

    push_scope(name)->var = var;

    return var;

}

void push_tag_scope(Token *tok, Type *ty) {
    TagScope *sc = calloc(1, sizeof(TagScope));
    sc->next = tag_scope;
    sc->name = strndup(tok->str, tok->len);
    sc->ty = ty;
    tag_scope = sc;
}

char *new_label(void) {
    static int cnt = 0;
    char buf[20];
    sprintf(buf, ".L.data.%d", cnt++);
    return strndup(buf, 20);
}

Function *function(void);
Type *type_specifier(void);
Type *declarator(Type *ty, char **name);
Type *type_suffix(Type *ty);
Type *struct_decl(void);
Member *struct_member(void);
void global_var(void);
Node *declaration(void);
bool is_typename(void);
Node *stmt(void);
Node *expr(void);
Node *assign(void);
Node *equality(void);
Node *relational(void);
Node *add(void);
Node *mul(void);
Node *unary(void);
Node *postfix(void);
Node *primary(void);

bool is_function(void) {
    Token *tok = token;

    Type *ty = type_specifier();
    char *name = NULL;
    declarator(ty, &name);
    bool is_func = name && consume("(");

    token = tok;
    return is_func;
}

Program *program(void) {
    Function head;
    head.next = NULL;
    Function *cur = &head;
    globals = NULL;
    
    while (!at_eof()) {
        if (is_function()) {
        cur->next = function();
        cur = cur->next;
        }
        else {
            global_var();
        }
    }

    Program *prog = calloc(1, sizeof(Program));
    prog->globals = globals;
    prog->fns = head.next;
    return prog;
}

// type-specifier = builtin-type | struct-decl | typedef-name
// builtin-type   = "void"
//                | "_Bool"
//                | "char"
//                | "short" | "short" "int" | "int" "short"
//                | "int"
//                | "long" | "long" "int" | "int" "long"
//
// Note that "typedef" can appear anywhere in a type-specifier.
Type *type_specifier(void) {
    if (!is_typename()) {
        error_tok(token, "typename expected");
    }

    Type *ty = NULL;
    enum {
        VOID = 1 << 1,
        BOOL = 1 << 3,
        CHAR = 1 << 5,
        SHORT = 1 << 7,
        INT = 1 << 9,
        LONG = 1 << 11,
    };

    int base_type = 0;
    Type *user_type = NULL;

    bool is_typedef = false;
    for (;;)
    {
        Token *tok = token;
        if (consume("typedef")) {
            is_typedef = true;
        } else if(consume("void")) {
            base_type += VOID;
        } else if(consume("_Bool")) {
            base_type += BOOL;
        } else if(consume("char")) {
            base_type += CHAR;
        } else if(consume("int")) {
            base_type += INT;
        } else if(consume("short")) {
            base_type += SHORT;
        } else if(consume("long")) {
            base_type += LONG;
        } else if(peek("struct")) {
            if (base_type || user_type) {
                break;
            }
            user_type = struct_decl();
        } else {
            if (base_type || user_type) {
                break;
            }
            Type *ty = find_typedef(token);
            if (!ty) {
                break;
            }
            token = token->next;
            user_type = ty;
        }
        switch (base_type) {
            case VOID:
                ty = void_type();
                break;
            case BOOL:
                ty = bool_type();
                break;
            case CHAR:
                ty = char_type();
                break;
            case SHORT:
            case SHORT + INT:
                ty = short_type();
                break;
            case INT:
                ty = int_type();
                break;
            case LONG:
            case LONG + INT:
                ty = long_type();
                break;
            case 0:
            {
                // If there's no type specifier, it becomes int.
                // For example, `typedef x` defines x as an alias for int.
                ty = user_type ? user_type : int_type();
                break;
            }
            default:
                error_tok(tok, "invalid type");
        }
    }

    ty->is_typedef = is_typedef;
    return ty;
}

Type *declarator(Type *ty, char **name) {
    while (consume("*")) {
        ty = pointer_to(ty);
    }

    if (consume("(")) {
        Type *placeholder = calloc(1, sizeof(Type));
        Type *new_ty = declarator(placeholder, name);
        expect(")");
        *placeholder = *type_suffix(ty);
        return new_ty;
    }

    *name = expect_ident();
    return type_suffix(ty);
}



// type-suffix = ("[" num "]" type-suffix)?
Type *type_suffix(Type *ty) {
    if (!consume("[")) {
        return ty;
    }
    int sz = expect_number();
    expect("]");
    ty = type_suffix(ty);
    return array_of(ty, sz);
}

Type *struct_decl(void) {
    // Read a struct tag.
    expect("struct");
    Token *tag = consume_ident();
    if (tag && !peek("{")) {
        TagScope *sc = find_tag(tag);
        if (!sc) {
            error_tok(tag, "unknown struct type");
        }
        return sc->ty;
    }
    expect("{");

    Member head;
    head.next = NULL;
    Member *cur = &head;

    while (!consume("}")) {
        cur->next = struct_member();
        cur = cur->next;
    }

    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_STRUCT;
    ty->members = head.next;

    int offset = 0;
    for (Member *mem = ty->members; mem; mem = mem->next) {
        offset = align_to(offset, mem->ty->align);
        mem->offset = offset;
        offset += size_of(mem->ty);

        if (ty->align < mem->ty->align) {
            ty->align = mem->ty->align;
        }
    }

    if (tag) {
        push_tag_scope(tag, ty);
    }

    return ty;
}

// struct-member = type-specifier declarator type-suffix ";"
Member *struct_member(void) {
    Type *ty = type_specifier();
    char *name = NULL;
    ty = declarator(ty, &name);
    ty = type_suffix(ty);
    expect(";");

    Member *mem = calloc(1, sizeof(Member));
    mem->ty = ty;
    mem->name = name;
    return mem;
}

VarList *read_func_param(void) {
    Type *ty = type_specifier();
    char *name = NULL;
    ty = declarator(ty, &name);
    ty = type_suffix(ty);

    VarList *vl = calloc(1, sizeof(VarList));
    vl->var = push_var(ty, name, true);
    return vl;
}

VarList *read_func_params(void) {
    if (consume(")")) {
        return NULL;
    }

    VarList *head = read_func_param();
    VarList *cur = head;

    while (!consume(")")) {
        expect(",");
        cur->next = read_func_param();
        cur = cur->next;
    }
    return head;
}

// function = type-specifier declarator "(" params? ")" "{" stmt* "}"
// params   = param ("," param)*
// param    = type-specifier declarator type-suffix
Function *function(void) {
    locals = NULL;

    Type *ty = type_specifier();
    char *name = NULL;
    ty = declarator(ty, &name);

    push_var(func_type(ty), name, false);

    Function *fn = calloc(1, sizeof(Function));
    fn->name = name;
    expect("(");
    fn->params = read_func_params();
    expect("{");

    Node head;
    head.next = NULL;
    Node *cur = &head;

    while (!consume("}")) {
        cur->next = stmt();
        cur = cur->next;
    }

    fn->node = head.next;
    fn->locals = locals;

    return fn;

}

// global-var = type-specifier declarator type-suffix ";"
void global_var(void) {
    Type *ty = type_specifier();
    char *name = NULL;
    ty = declarator(ty, &name);
    ty = type_suffix(ty);
    expect(";");
    push_var(ty, name, false);
}

// declaration = type-specifier declarator type-suffix ("=" expr)? ";"
//             | type-specifier ";"
Node *declaration(void) {
    Token *tok = token;
    Type *ty = type_specifier();

    if (consume(";")) {
        return new_node(ND_NULL, tok);
    }
    char *name = NULL;
    ty = declarator(ty, &name);
    ty = type_suffix(ty);

    if (ty->is_typedef) {
        expect(";");
        ty->is_typedef = false;
        push_scope(name)->type_def = ty;
        return new_node(ND_NULL, tok);
    }
    if (ty->kind == TY_VOID) {
        error_tok(tok, "variable declared void");
    }
    Var *var = push_var(ty, name, true);

    if (consume(";")) {
        return new_node(ND_NULL, tok);
    }
    expect("=");
    Node *lhs = new_node_Var(var, tok);
    Node *rhs = expr();
    expect(";");
    Node *node = new_binary(ND_ASSIGN, lhs, rhs, tok);
    return new_unary(ND_EXPR_STMT, node, tok);
}

Node *read_expr_stmt() {
  Token *tok = token;
  return new_unary(ND_EXPR_STMT, expr(), tok);
}

bool is_typename(void) {
    return peek("_Bool") || peek("void") || peek("char") || peek("short") || peek("int") || peek("long") ||
        peek("struct") || peek("typedef") || find_typedef(token);
}

// stmt = "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "{" stmt* "}"
//      | declaration
//      | expr ";"
Node *stmt(void) {
    Node *node;

    Token *tok;
    if (tok = consume("return")) {
        node = new_unary(ND_RETURN, expr(), tok);
        expect(";");
        return node;
    }
    
    if (tok = consume("if")) {
        node = new_node(ND_IF, tok);
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        node->els = NULL;
        if (consume("else")) {
            node->els = stmt();
        }
        return node;
    }

    if (tok = consume("while")) {
        node = new_node(ND_WHILE, tok);
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        return node;
    }

    if (tok = consume("for")) {
        node = new_node(ND_FOR, tok);
        expect("(");
        if (!consume(";")) {
            node->init = read_expr_stmt();
            expect(";");
        }
        if (!consume(";")) {
            node->cond = expr();
            expect(";");
        }
        if (!consume(")")) {
            node->inc = read_expr_stmt();
            expect(")");
        }
        node->then = stmt();
        return node;
    }

    if (tok = consume("{")) {
        Node head;
        head.next = NULL;
        Node *cur = &head;

        VarScope *sc_var = var_scope;
        TagScope *sc_tag = tag_scope;

        while (!consume("}")) {
            cur->next = stmt();
            cur = cur->next;
        }
        var_scope = sc_var;
        tag_scope = sc_tag;

        Node *node = new_node(ND_BLOCK, tok);
        node->body = head.next;

        return node;
    }

    if (is_typename()) {
        return declaration();
    }

    node = read_expr_stmt();
    expect(";");
    return node;
}

// creates expr := assign
Node *expr(void) {
    return assign();
}

// creates assign := equality ("=" assign)?
Node *assign(void) {
    Node *node = equality();
    Token *tok;
    if (tok = consume("=")) {
        node = new_binary(ND_ASSIGN, node, assign(), tok);
    }
    return node;
}

// creates equality := relational ("==" relational | "!=" relational)*
Node *equality(void) {
    Node *node = relational();
    Token *tok;
    for (; ; ) {
        if (tok = consume("==")) {
            node = new_binary(ND_EQ, node, relational(), tok);
        }
        else if (tok = consume("!=")) {
            node = new_binary(ND_NE, node, relational(), tok);
        }
        else {
            return node;
        }
    }
}

// creates relational := add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational(void) {
    Node *node = add();
    Token *tok;
    for (; ; ) {
        if (tok = consume("<")) {
            node = new_binary(ND_LT, node, add(), tok);
        }
        else if (tok = consume("<=")) {
            node = new_binary(ND_LE, node, add(), tok);
        }
        else if (tok = consume(">")) {
            node = new_binary(ND_LT, add(), node, tok);
        }
        else if (tok = consume(">=")) {
            node = new_binary(ND_LE, add(), node, tok);
        }
        else {
            return node;
        }
    }
}

// creates add := mul ("+" mul | "-" mul)*
Node *add(void) {
    Node *node = mul();
    Token *tok;
    for (; ; ) {
        if (tok = consume("+")){
            node = new_binary(ND_ADD, node, mul(), tok);
        }
        else if (tok = consume("-")) {
            node = new_binary(ND_SUB, node, mul(), tok);
        }
        else {
            return node;
        }
    }
}

// creates mul := unary ("*" unary|"/" unary)*
Node *mul(void) {
    Node *node = unary();
    Token *tok;
    for (; ; ) {
        if (tok = consume("*")){
            node = new_binary(ND_MUL, node, unary(), tok);
        }
        else if (tok = consume("/")) {
            node = new_binary(ND_DIV, node, unary(), tok);
        }
        else {
            return node;
        }
    }
}

// creates unary := "sizeof" unary
//                | "+"? unary
//                | "-"? unary
//                | "*"? unary
//                | "&"? unary
//                | postfix
Node *unary(void) {
    Token *tok;
    if (consume("+")) {
        return unary();
    }
    if (tok = consume("-")) {
        return new_binary(ND_SUB, new_node_num(0, tok), unary(), tok);
    }
    if (tok = consume("&")) {
        return new_unary(ND_ADDR, unary(), tok);
    }
    if (tok = consume("*")) {
        return new_unary(ND_DEREF, unary(), tok);
    }
    
    return postfix();
}

// postfix = primary ("[" expr "]" | "." ident | "->" ident)*
Node *postfix(void) {
    Node *node = primary();
    Token *tok;
    for(;;) {
        if (tok = consume("[")) {
            Node *exp = new_binary(ND_ADD, node, expr(), tok);
            expect("]");
            node = new_unary(ND_DEREF, exp, tok);
            continue;
        }

        if (tok = consume(".")) {
            node = new_unary(ND_MEMBER, node, tok);
            node->member_name = expect_ident();
            continue;
        }
        if (tok = consume("->")) {
            node = new_unary(ND_DEREF, node, tok);
            node = new_unary(ND_MEMBER, node, tok);
            node->member_name = expect_ident();
            continue;
        }

        return node;
    }
    
}

// stmt-expr = "(" "{" stmt stmt* "}" ")"
//
// Statement expression is a GNU C extension.
Node *stmt_expr(Token *tok) {
    VarScope *sc_var = var_scope;
    TagScope *sc_tag = tag_scope;

    Node *node = new_node(ND_STMT_EXPR, tok);
    node->body = stmt();
    Node *cur = node->body;

    while (!consume("}")) {
        cur->next = stmt();
        cur = cur->next;
    }
    expect(")");

    var_scope = sc_var;
    tag_scope = sc_tag;
    if (cur->kind != ND_EXPR_STMT)
    error_tok(cur->tok, "stmt expr returning void is not supported");
    *cur = *cur->lhs;
    return node;
}

// func-args = "(" (assign ("," assign)*)? ")"
Node *func_args(void) {
    if (consume(")")) {
        return NULL;
    }

    Node *head = assign();
    Node *cur = head;
    while (consume(",")) {
        cur->next = assign();
        cur = cur->next;
    }
    expect(")");
    return head;
}

// primary = "(" "{" stmt-expr-tail
//         | "(" expr ")"
//         | "sizeof" unary
//         | ident func-args?
//         | str
//         | num
Node *primary(void) {
    Token *tok;
    if (tok = consume("(")) {
    if (consume("{"))
      return stmt_expr(tok);

    Node *node = expr();
    expect(")");
    return node;
  }
  
    if (tok = consume("sizeof")){
        return new_unary(ND_SIZEOF, unary(), tok);
    }

    if (tok = consume_ident()) {
        if (consume("(")) {
            Node *node = new_node(ND_FUNCALL, tok);
            node->funcname = strndup(tok->str, tok->len);
            node->args = func_args();

            VarScope *sc = find_Var(tok);
            if (sc) {
                if (!sc->var || sc->var->ty->kind != TY_FUNC) {
                    error_tok(tok, "not a function");
                }
                node->ty = sc->var->ty->return_ty;
            } else {
                node->ty = int_type();
            }
            return node;
        }

        VarScope *sc = find_Var(tok);
        if (sc && sc->var) {
            return new_node_Var(sc->var, tok);
        }
        error_tok(tok, "undefined variable");
    }
    
    tok = token;
    if (tok->kind == TK_STR) {
        token = token->next;
        Type *ty = array_of(char_type(), tok->cont_len);
        Var *var = push_var(ty, new_label(), false);
        var->contents = tok->contents;
        var->cont_len = tok->cont_len;
        return new_node_Var(var, tok);
    }
    
    if (tok->kind != TK_NUM) {
        error_tok(tok, "expected expression");
    }
    return new_node_num(expect_number(), tok);
}