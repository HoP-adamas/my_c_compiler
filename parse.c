#include "9cc.h"

VarList *locals;
VarList *globals;
VarList *scope;

char *new_label(void) {
    static int cnt = 0;
    char buf[20];
    sprintf(buf, ".Ldata.%d", cnt++);
    return strndup(buf, 20);
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

Node *new_node_Var(Var *var) {
    Node *node = new_node(ND_VAR, NULL, NULL);
    node->var = var;
    return node;
}

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

Var *push_var(Type *ty, char *name, bool is_local) {
    Var *var = calloc(1, sizeof(Var));
    var->ty = ty;
    var->name = name;
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
    sc-> next = scope;
    scope = sc;

    return var;

}

Type *basetype(void) {
    Type *ty;
    if (consume("char")) {
        ty = char_type();
    }
    else {
        expect("int");
        ty = int_type();
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

/* BNF:
   program    = stmt*
   stmt       = expr ";"
              | "{" stmt* "}"
              | "if" "(" expr ")" stmt ("else" stmt)?
              | "while" "(" expr ")" stmt
              | "for" "(" expr? ";" expr? ";" expr? ")" stmt
   expr       = assign
   assign     = equality ("=" assign)?
   equality   = relational ("==" relational | "!=" relational)*
   relational = add ("<" add | "<=" add | ">" add | ">=" add)*
   add        = mul ("+" mul | "-" mul)*
   mul        = unary ("*" unary | "/" unary)*
   unary      = ("+" | "-")? primary
   primary    = num
              | ident ("(" ")")?
              | "(" expr ")"
 */


// creates expr := assign
Node *expr(void) {
    return assign();
}

bool is_typename(void) {
    return peek("int") || peek("char");
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
        node = new_node(ND_RETURN, expr(), NULL);
        expect(";");
        return node;
    }
    
    if (tok = consume("if")) {
        expect("(");
        node = calloc(1, sizeof(Node));
        node->kind = ND_IF;
        node->cond = expr();
        expect(")");
        node->then = stmt();
        node->els = NULL;
        if (tok = consume("else")) {
            node->els = stmt();
        }
        return node;
    }

    if (tok = consume("while")) {
        expect("(");
        node = calloc(1, sizeof(Node));
        node->kind = ND_WHILE;
        node->cond = expr();
        expect(")");
        node->body = stmt();
        return node;
    }

    if (tok = consume("for")) {
        expect("(");
        node = calloc(1, sizeof(Node));
        node->kind = ND_FOR;
        node->init = NULL;
        node->cond = NULL;
        node->inc = NULL;
        if (!consume(";")) {
            node->init = expr();
            expect(";");
        }
        if (!consume(";")) {
            node->cond = expr();
            expect(";");
        }
        if (!consume(")")) {
            node->inc = expr();
            expect(")");
        }
        node->body = stmt();
        return node;
    }

    if(consume("{")) {
        Node head;
        head.next = NULL;
        Node *cur = &head;

        VarList *sc = scope;

        while (!consume("}")) {
            cur->next = stmt();
            cur = cur->next;
        }
        scope = sc;

        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_BLOCK;
        node->body = head.next;

        return node;
    }

    if(is_typename()) {
        return declaration();
    }

    
    node = new_node(ND_EXPR_STMT, expr(), NULL);
    if (!consume(";")) {
        error_at(token->str, "this token is not ';'");
    }
    return node;
}

bool is_function(void) {
    Token *tok = token;
    basetype();

    bool is_func = consume_ident() && consume("(");
    token = tok;
    return is_func;
}

void global_var(void) {
    Type *ty = basetype();
    char *name = expect_ident();
    ty = read_type_suffix(ty);
    expect(";");
    push_var(ty, name, false);
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

// declaration := basetype ident ("[" num "]")* ("=" expr) ";"
Node *declaration(void) {
    Type *ty = basetype();
    char *name = expect_ident();
    ty = read_type_suffix(ty);
    Var *var = push_var(ty, name, true);

    if (consume(";")) {
        return new_node(ND_NULL, NULL, NULL);
    }
    expect("=");
    Node *lhs = new_node_Var(var);
    Node *rhs = expr();
    expect(";");
    Node *node = new_node(ND_ASSIGN, lhs, rhs);
    return new_node(ND_EXPR_STMT, node, NULL);
}

// creates assign := equality ("=" assign)?
Node *assign(void) {
    Node *node = equality();
    if (consume("=")) {
        node = new_node(ND_ASSIGN, node, assign());
    }
    return node;
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

// creates unary := "sizeof" unary
//                | "+"? unary
//                | "-"? unary
//                | "*"? unary
//                | "&"? unary
//                | postfix
Node *unary(void) {
    if (consume("+")) {
        return unary();
    }
    if (consume("-")) {
        return new_node(ND_SUB, new_node_num(0), unary());
    }
    if (consume("*")) {
        return new_node(ND_DEREF, unary(), NULL);
    }
    if (consume("&")) {
        return new_node(ND_ADDR, unary(), NULL);
    }
    return postfix();
}

// postfix = primary ("[" expr "]")*
Node *postfix(void) {
    Node *node = primary();
    
    while (consume("[")) {
        Node *exp = new_node(ND_ADD, node, expr());
        expect("]");
        node = new_node(ND_DEREF, exp, NULL);
    }
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

// primary = "(" expr ")" | "sizeof" unary | ident func-args? | str | num
// args = "(" ident ("," ident)* ")"
Node *primary(void) {
    if (consume("sizeof")){
        return new_node(ND_SIZEOF, unary(), NULL);
    }
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }
    
    Token *tok = consume_ident();
    if (tok) {
        
        if (consume("(")) {
            Node *node = calloc(1, sizeof(Node));
            node->kind = ND_FUNCALL;
            node->funcname = strndup(tok->str, tok->len);
            node->args = func_args();
            return node;
        }

        Var *var = find_Var(tok);
        if (!var) {
            // var = push_var(strndup(tok->str, tok->len));
            error("undefined variable &s", tok->str);
        }
        return new_node_Var(var);
    }
    tok = token;
    if (tok->kind == TK_STR) {
        token = token->next;
        Type *ty = array_of(char_type(), tok->cont_len);
        Var *var = push_var(ty, new_label(), false);
        var->contents = tok->contents;
        var->cont_len = tok->cont_len;
        return new_node_Var(var);
    }
    
    return new_node_num(expect_number());

    

    error("expected an expression\n");
}