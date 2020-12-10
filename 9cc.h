#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


// kinds of tokens
typedef enum {
    TK_RESERVED,    // symbol
    TK_NUM,         // Integer Token
    TK_EOF,         // Token of End Of File

} TokenKind;

typedef struct Token Token;


struct Token {
    TokenKind kind; // kind of token
    Token *next;    // next input token
    int val;        // the value of token when the kind is TK_NUM
    char *str;      // token string
};


void error(char *fmt, ...);
bool consume(char op);
void expect(char op);
int expect_number(void);
bool at_eof(void);
Token *new_token(TokenKind kind, Token *cur, char *str);
Token *tokenize(char *p);