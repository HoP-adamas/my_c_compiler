#include "9cc.h"

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

Node *new_node_lvar(char name) {
    Node *node = new_node(ND_LVAR, NULL, NULL);
    node->offset = (name - 'a' + 1)*8;
    // node->name = name;
    return node;
}




// creates expr := assign
Node *expr(void) {
    return assign();
}

// stmt := expr ";"
Node *stmt(void) {
    Node *node = expr();
    expect(";");
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
        return new_node_lvar(*tok->str);
    }

    return new_node_num(expect_number());
}