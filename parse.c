#include "9cc.h"

LVar *locals;
Node *code[100];

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

Node *new_node_lvar(LVar *var) {
    Node *node = new_node(ND_LVAR, NULL, NULL);
    node->var = var;
    return node;
}

// search variable by its name. if it is not be found, return NULL.
LVar *find_LVar(Token *tok) {

    for (LVar *var = locals; var; var = var->next) {
        if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
            return var;
        }
    }
    return NULL;
}

/* BNF:
   program    = stmt*
   stmt       = expr ";"
              | "if" "(" expr ")" stmt ("else" stmt)?
              | "while" "(" expr ")" stmt
              | "for" "(" expr? ";" expr? ";" expr? ")" stmt
   expr       = assign
   assign     = equality ("=" assign)?
   equality   = relational ("==" relational | "!=" relational)*
   relational = add ("<" add | "<=" add | ">" add | ">=" add)*
   add        = mul ("+" mul | "-" mul)*
   mul        = unary ("*" unary | "/" unary)*
   unary      = ("+" | "-")? term
   term       = num | var | "(" expr ")"
   var        = ("a" ～ "z") ("a" ～ "z" | "A" ～ "Z" | "0" ～ "9" | "_")*
   num        = int ("0" | int)*
   int        = "1" ～ "9"
 */


// creates expr := assign
Node *expr(void) {
    return assign();
}

/* stmt    = expr ";"
           | "if" "(" expr ")" stmt ("else" stmt)?
           | "while" "(" expr ")" stmt
           | "for" "(" expr? ";" expr? ";" expr? ")" stmt

*/
Node *stmt(void) {
    Node *node;

    Token *tok = consume_if();
    if (tok) {
        expect("(");
        node = calloc(1, sizeof(Node));
        node->kind = ND_IF;
        node->lhs = expr();
        expect(")");
        node->rhs = stmt();
        tok = consume_else();
        node->els = NULL;
        if (tok) {
            node->els = stmt();
        }
        return node;
    }

    tok = consume_while();
    if (tok) {
        expect("(");
        node = calloc(1, sizeof(Node));
        node->kind = ND_WHILE;
        node->lhs = expr();
        expect(")");
        node->rhs = stmt();
        return node;
    }
    tok = consume_return();

    if (tok) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_RETURN;
        node->lhs = expr();
    }
    else {
        node = expr();
    }
    
    if (!consume(";")) {
        error_at(token->str, "this token is not ';'");
    }
    return node;
}

void program(void) {
    int i = 0;
    while (!at_eof()) {
        code[i++] = stmt();
    }
    code[i] = NULL;
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

// creates primary := num | ident | "(" expr ")"
Node *primary(void) {
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }
    // TODO add ident
    Token *tok = consume_ident();
    if (tok) {

        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_LVAR;

        LVar *lvar = find_LVar(tok);
        if (lvar) {
            node->offset = lvar->offset;
        }
        else {
            lvar = calloc(1,sizeof(LVar));
            lvar->next = locals;
            lvar->name = tok->str;
            lvar->len = tok->len;
            if (locals == NULL) {
                lvar->offset = 8;
            }
            else {
                lvar->offset = locals->offset + 8;
            }
            node->offset = lvar->offset;
            locals = lvar;
        }
        return node;
    }

    if (token->kind == TK_NUM) {
        Node *node = new_node_num(token->val);
        token = token->next;
        return node;
    }

    error("expected an expression\n");
}