#include "9cc.h"

VarList *locals;

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
    Node *node = new_node(ND_Var, NULL, NULL);
    node->var = var;
    return node;
}

// search variable by its name. if it is not be found, return NULL.
Var *find_Var(Token *tok) {

    for (VarList *vl = locals; vl; vl = vl->next) {
        Var *var = vl->var;
        if (strlen(var->name) == tok->len && !memcmp(tok->str, var->name, tok->len)) {
            return var;
        }
    }
    return NULL;
}

Var *push_var(Type *ty, char *name) {
    Var *var = calloc(1, sizeof(Var));
    var->ty = ty;
    var->name = name;
    VarList *vl = calloc(1, sizeof(VarList));
    vl->next = locals;
    vl->var = var;
    locals = vl;
    return var;

}

Type *basetype(void) {
    expect("int");
    Type *ty = int_type();

    return ty;

}

VarList *read_func_param(void) {
    VarList *vl = calloc(1, sizeof(VarList));
    Type *ty = basetype();
    vl->var = push_var(ty, expect_ident());
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

        while (!consume("}")) {
            cur->next = stmt();
            cur = cur->next;
        }

        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_BLOCK;
        node->body = head.next;

        return node;
    }

    if(tok = peek("int")) {
        return declaration();
    }

    
    node = new_node(ND_EXPR_STMT, expr(), NULL);
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

// declaration = basetype ident ("=" expr)?;
Node *declaration(void) {
    Type *ty = basetype();
    Var *var = push_var(ty, expect_ident());

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

// creates unary := "+"? unary
//                | "-"? unary
//                | "*"? unary
//                | "&"? unary
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
    
    return new_node_num(expect_number());

    

    error("expected an expression\n");
}