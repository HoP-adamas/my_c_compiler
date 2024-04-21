#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <errno.h>


typedef struct Type Type;
typedef struct Member Member;




// kinds of tokens
typedef enum {
    TK_RESERVED,    // symbol
    TK_IDENT,       // identifier
    TK_STR,         // string Token
    TK_NUM,         // Integer Token
    TK_EOF,         // Token of End Of File

} TokenKind;

typedef struct Token Token;


struct Token {
    TokenKind kind; // kind of token
    Token *next;    // next input token
    long val;        // the value of token when the kind is TK_NUM
    char *str;      // token string
    int len;        // length of token

    char *contents; // contents of string token including '\0'
    char cont_len;  //length of contents of string token
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);

Token *peek(char *s);
Token *consume(char *s);
Token *consume_ident(void);
void expect(char *op);
long expect_number(void);
char *expect_ident(void);
bool at_eof(void);
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
Token *tokenize(char *p);

extern char *filename;
extern char *user_input;
extern Token *token;
typedef struct Var Var;

struct Var {
    char *name;     // the name of local variable
    int offset;     // the offset from RBP
    Type *ty;

    bool is_local;  // local or global

    char *contents;
    int cont_len;
};

// Local variable


typedef struct VarList VarList;

struct VarList {
    VarList *next;
    Var *var;
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
    ND_MEMBER,  // . (struct member access)
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
    ND_STMT_EXPR, // Statement expression
    ND_ADDR,    // unary &
    ND_DEREF,   // unary *
    ND_NULL,    // Empty statement
    ND_SIZEOF,  // sizeof
} NodeKind;

typedef struct Node Node;

// type of node of AST
struct Node {
    NodeKind kind;  // type of the node
    Node *next;     // next node
    Node *lhs;      // left-hand side of the node
    Node *rhs;      // right-hand side of the node
    long val;        // use only if kind is ND_NUM
    int offset;     // use only if kind is ND_VAR
    Var *var;
    Token *tok;

    // "if" ( cond ) then "else" els
    // "for" ( init; cond; inc ) body
    // "while" ( cond ) body
    Node *cond;
    Node *then;
    Node *els;
    Node *init;
    Node *inc;

     // Block or statement expression
    Node *body;

    // Struct member access
    char *member_name;
    Member *member;

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
    Node *node;
    VarList *locals;
    int stack_size;
};

typedef struct {
    VarList *globals;
    Function *fns;
} Program;

Program *program();


typedef enum {
    TY_VOID,
    TY_BOOL,
    TY_SHORT,
    TY_INT,
    TY_LONG,
    TY_CHAR,
    TY_PTR,
    TY_ARRAY,
    TY_STRUCT,
    TY_FUNC,
} TypeKind;

struct Type
{
    TypeKind kind;
    bool is_typedef;    // typedef
    int align;          // alignment
    Type *base;         // pointer or array
    size_t array_size;  // array
    Member *members;    // struct
    Type *return_ty;    // function
};

// struct member
struct Member {
    Member *next;
    Type *ty;
    char *name;
    int offset;
};

int align_to(int n, int align);
Type *void_type(void);
Type *bool_type(void);
Type *short_type(void);
Type *int_type(void);
Type *long_type(void);
Type *char_type(void);
Type *func_type(Type *return_ty);
Type *pointer_to(Type *base);
Type *array_of(Type *base, int size);
int size_of(Type *ty);
void add_type(Program *prog);


// codegen.c
void gen(Node *node);
void codegen(Program *prog);


extern Function *prog;

extern VarList *locals;

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