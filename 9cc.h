#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>


typedef struct Type Type;
typedef struct 
{
    void **data;
    int capacity;
    int len;
} Vector;

Vector *new_vec(void);
void vec_push(Vector *v, void *elem);

typedef struct 
{
    Vector *keys;
    Vector *vals;
} Map;

Map *new_map(void);
void map_put(Map *map, char *key, void *val);
void *map_get(Map *map, char *key);

char *strndup(char *str, int chars);



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
    ND_VAR,    // local one letter variable
    ND_RETURN,  // return
    ND_IF,      // if
    ND_ELSE,    // else
    ND_WHILE,   // while
    ND_FOR,     // for
    ND_BLOCK,   // {...}
    ND_FUNCALL, // Function call
    ND_EXPR_STMT, // Expression statement
    ND_ADDR,    // unary &
    ND_DEREF,   // unary *
    ND_NULL,    // Empty statement
    ND_SIZEOF,  // sizeof
} NodeKind;

// Local variable
typedef struct Var Var;

struct Var {
    char *name;     // the name of local variable
    int offset;     // the offset from RBP
    Type *ty;

    bool is_local;  // local or global
};

typedef struct VarList VarList;

struct VarList {
    VarList *next;
    Var *var;
};

typedef struct Node Node;

// type of node of AST
struct Node {
    NodeKind kind;  // type of the node
    Node *next;     // next node
    Node *lhs;      // left-hand side of the node
    Node *rhs;      // right-hand side of the node
    int val;        // use only if kind is ND_NUM
    int offset;     // use only if kind is ND_VAR
    Var *var;

    // "if" ( cond ) then "else" els
    // "for" ( init; cond; inc ) body
    // "while" ( cond ) body
    Node *cond;
    Node *then;
    Node *els;
    Node *init;
    Node *inc;
    Node *body;

    // Function call
    char *funcname;
    Node *args;

    Type *ty;
};

typedef struct  Function Function;

struct Function {
    Function *next;
    char *name;
    VarList *params;
    VarList *locals;
    Node *node;
    int stack_size;
};

typedef struct
{
    VarList *globals;
    Function *fns;
} Program;




void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
Token *peek(char *s);
Token *consume(char *s);
Token *consume_ident(void);
void expect(char *op);
int expect_number(void);
char *expect_ident(void);
bool at_eof(void);
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
Token *tokenize(char *p);

// parse.c
Var *find_Var(Token *tok);
Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);
Node *new_node_Var(Var *var);
Node *stmt(void);
Program *program(void);
Function *function(void);
Node *declaration(void);
Node *expr(void);
Node *assign(void);
Node *equality(void);
Node *relational(void);
Node *add(void);
Node *mul(void);
Node *unary(void);
Node *postfix(void);
Node *primary(void);

typedef enum {
    TY_INT,
    TY_CHAR,
    TY_PTR,
    TY_ARRAY,
} TypeKind;

struct Type
{
    TypeKind kind;
    Type *base;
    size_t array_size;
};

Type *int_type(void);
Type *char_type(void);
Type *pointer_to(Type *base);
Type *array_of(Type *base, int size);
int size_of(Type *ty);
void add_type(Program *prog);


// codegen.c
void gen(Node *node);
void codegen(Program *prog);

// the token we focus on
extern Token *token;

extern char *user_input;

extern Function *prog;

extern VarList *locals;
