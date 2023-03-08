#include "9cc.h"


// Scope for struct tags
typedef struct TagScope TagScope;
struct TagScope {
    TagScope *next;
    char *name;
    Type *ty;
};
VarList *locals;
VarList *globals;
VarList *scope;
TagScope *tag_scope;

// search variable by its name. if it is not be found, return NULL.
Var *find_Var(Token *tok) {

    // find a variable by its name.
    for (VarList *vl = scope; vl; vl = vl->next) {
        Var *var = vl->var;
        if (strlen(var->name) == tok->len && !memcmp(tok->str, var->name, tok->len)) {
            return var;
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
    else {
        vl->next = globals;
        globals = vl;
    }

    VarList *sc = calloc(1, sizeof(VarList));
    sc->var = var;
    sc->next = scope;
    scope = sc;

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
Type *basetype(void);
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
    basetype();

    bool is_func = consume_ident() && consume("(");
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

// basetype = ("char" | "int" | struct-decl) "*"*
Type *basetype(void) {
    Type *ty;
    if (consume("char")) {
        ty = char_type();
    }
    else if (consume("int")) {
        ty = int_type();
    } else {
        ty = struct_decl(); 
    }

    while (consume("*")) {
        ty = pointer_to(ty);
    }

    return ty;

}

Type *read_type_suffix(Type *base) {
    if (!consume("[")) {
        return base;
    }
    int sz = expect_number();
    expect("]");
    base = read_type_suffix(base);
    return array_of(base, sz);
}

Type *struct_decl(void) {
    expect("struct");

    // Read a struct tag.
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

Member *struct_member(void) {
    Member *mem = calloc(1, sizeof(Member));
    mem->ty = basetype();
    mem->name = expect_ident();
    mem->ty = read_type_suffix(mem->ty);
    expect(";");
    return mem;
}

VarList *read_func_param(void) {
    Type *ty = basetype();
    char *name = expect_ident();
    ty = read_type_suffix(ty);

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

// function = basetype ident "(" params? ")" "{" stmt* "}"
// params = param ("," param)*
// param = basetype ident;
Function *function(void) {
    locals = NULL;
    Function *fn = calloc(1, sizeof(Function));
    basetype();
    fn->name = expect_ident();
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

void global_var(void) {
    Type *ty = basetype();
    char *name = expect_ident();
    ty = read_type_suffix(ty);
    expect(";");
    push_var(ty, name, false);
}

// declaration := basetype ident ("[" num "]")* ("=" expr) ";"
//              | basetype ";"
Node *declaration(void) {
    Token *tok = token;
    Type *ty = basetype();
    if (consume(";")) {
        return new_node(ND_NULL, tok);
    }
    char *name = expect_ident();
    ty = read_type_suffix(ty);
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
    return peek("int") || peek("char") || peek("struct");
}

/* stmt    = "return" expr ";"
           | "{" stmt* "}"
           | "if" "(" expr ")" stmt ("else" stmt)?
           | "while" "(" expr ")" stmt
           | "for" "(" expr? ";" expr? ";" expr? ")" stmt
           | declaration
           | expr ";"

*/
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

        VarList *sc_var = scope;
        TagScope *sc_tag = tag_scope;

        while (!consume("}")) {
            cur->next = stmt();
            cur = cur->next;
        }
        scope = sc_var;
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

// postfix = primary ("[" expr "]" | "." ident)*
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

        return node;
    }
    
}

// stmt-expr = "(" "{" stmt stmt* "}" ")"
//
// Statement expression is a GNU C extension.
Node *stmt_expr(Token *tok) {
    VarList *sc_var = scope;
    TagScope *sc_tag = tag_scope;

    Node *node = new_node(ND_STMT_EXPR, tok);
    node->body = stmt();
    Node *cur = node->body;

    while (!consume("}")) {
        cur->next = stmt();
        cur = cur->next;
    }
    expect(")");

    scope = sc_var;
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
            return node;
        }

        Var *var = find_Var(tok);
        if (!var) {
            // var = push_var(strndup(tok->str, tok->len));
            error_tok(tok, "undefined variable");
        }
        return new_node_Var(var, tok);
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