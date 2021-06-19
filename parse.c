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

Function *function(void);

// creates expr := assign
Node *expr(void) {
    return assign();
}

/* stmt    = expr ";"
           | "{" stmt* "}"
           | "if" "(" expr ")" stmt ("else" stmt)?
           | "while" "(" expr ")" stmt
           | "for" "(" expr? ";" expr? ";" expr? ")" stmt

*/
Node *stmt(void) {
    Node *node;

    Token *tok;
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

        while (!consume("}")) {
            cur->next = stmt();
            cur = cur->next;
        }

        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_BLOCK;
        node->body = head.next;

        return node;
    }

    if (tok = consume("return")) {
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

Function *program(void) {
    Function head;
    head.next = NULL;
    Function *cur = &head;
    
    while (!at_eof()) {
        cur->next = function();
        cur = cur->next;
    }
    return head.next;
}
// function = ident "(" ")" "{" stmt* "}"
Function *function(void) {
    locals = NULL;
    char *name = expect_ident();
    expect("(");
    expect(")");
    expect("{");

    Node head;
    head.next = NULL;
    Node *cur = &head;

    while (!consume("}")) {
        cur->next = stmt();
        cur = cur->next;
    }

    Function *fn = calloc(1, sizeof(Function));
    fn->name = name;
    fn->node = head.next;
    fn->locals = locals;

    return fn;

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

// primary = "(" expr ")" | ident func-args? | num
Node *primary(void) {
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }
    // TODO add ident
    Token *tok = consume_ident();
    if (tok) {
        
        if (consume("(")) {
            Node *node = calloc(1, sizeof(Node));
            node->kind = ND_FUNCALL;
            node->funcname = strndup(tok->str, tok->len);
            node->args = func_args();
            return node;
        }
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