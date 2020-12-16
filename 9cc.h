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
    TK_NUM,         // Integer Token
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
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_EQ,  // ==
    ND_NE,  // !=
    ND_LT,  // <
    ND_LE,  // <=
    ND_NUM, // Integer
} NodeKind;

typedef struct Node Node;

// type of node of AST
struct Node {
    NodeKind kind;  // type of the node
    Node *lhs;      // left-hand side of the node
    Node *rhs;      // right-hand side of the node
    int val;        // use only if kind is ND_SUM
};


void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
bool consume(char *op);
void expect(char *op);
int expect_number(void);
bool at_eof(void);
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
Token *tokenize(char *p);

// parse.c
Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);
Node *expr(void);
Node *equality(void);
Node *relational(void);
Node *add(void);
Node *mul(void);
Node *unary(void);
Node *primary(void);


// codegen.c
void gen(Node *node);