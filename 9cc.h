#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


// kinds of tokens
typedef enum {
    TK_RESERVED,    // symbol
    TK_IDENT,       // identifier
    TK_IF,          // "if"
    TK_ELSE,        // "else"
    TK_WHILE,       // "while"
    TK_FOR,         // "for"
    TK_NUM,         // Integer Token
    TK_RETURN,      // token of return
    TK_EOF,         // Token of End Of File

} TokenKind;

typedef struct Token Token;


struct Token {
    TokenKind kind; // kind of token
    Token *next;    // next input token
    int val;        // the value of token when the kind is TK_NUM
    char *str;      // token string
    int len;        // length of token
};



// Kinds of node of abstruct syntax tree (AST)
typedef enum {
    ND_ADD,     // +
    ND_SUB,     // -
    ND_MUL,     // *
    ND_DIV,     // /
    ND_EQ,      // ==
    ND_NE,      // !=
    ND_LT,      // <
    ND_LE,      // <=
    ND_ASSIGN,  // =
    ND_NUM,     // Integer
    ND_LVAR,    // local one letter variable
    ND_RETURN,  // return
    ND_IF,      // if
    ND_ELSE,    // else
    ND_WHILE,   // while
    ND_FOR,     // for
} NodeKind;

// Local variable
typedef struct LVar LVar;

struct LVar {
    LVar *next;     // next local variable or NULL
    char *name;     // the name of local variable
    int len;        // the length of the name
    int offset;     // the offset from RBP
};
typedef struct Node Node;

// type of node of AST
struct Node {
    NodeKind kind;  // type of the node
    Node *lhs;      // left-hand side of the node
    Node *rhs;      // right-hand side of the node
    int val;        // use only if kind is ND_NUM
    int offset;     // use only if kind is ND_LVAR
    LVar *var;

    // "if" ( cond ) then "else" els
    // "for" ( init; cond; inc ) body
    // "while" ( cond ) body
    Node *cond;
    Node *then;
    Node *els;
    Node *init;
    Node *inc;
    Node *body;
    
};


void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
bool consume(char *op);
Token *consume_tokenKind(TokenKind);
void expect(char *op);
int expect_number(void);
bool at_eof(void);
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
Token *tokenize(char *p);

// parse.c
LVar *find_LVar(Token *tok);
Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);
Node *new_node_lvar(LVar *var);
Node *stmt(void);
void program(void);
Node *expr(void);
Node *assign(void);
Node *equality(void);
Node *relational(void);
Node *add(void);
Node *mul(void);
Node *unary(void);
Node *primary(void);


// codegen.c
void gen(Node *node);

// the token we focus on
extern Token *token;

extern char *user_input;

extern Node *code[100];

extern LVar *locals;