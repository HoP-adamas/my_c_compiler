#include "9cc.h"


// the token we focus on
Token *token;

// reports an error and exit.
// same args of printf()
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// If the next token is the symbol we expect,
// consume one token and return true else return false.
bool consume(char op) {
    if (token->kind != TK_RESERVED || token->str[0] != op) {
        return false;
    }
    token = token->next;
    return true;
}

// If the next token is the symbol we expect,
// consumes one token else reports an error.
void expect(char op) {
    if (token->kind != TK_RESERVED || token->str[0] != op) {
        error("next token is not '%c'", op);
    }
    token = token->next;
}

// If the next token  is a number,
// consumes one token and return this number else reports an error
int expect_number(void) {
    if (token->kind != TK_NUM) {
        error("next token is not a number");
    }
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof(void) {
    return token->kind == TK_EOF;
}

// creates a new token and connects as the next token of current token
Token *new_token(TokenKind kind, Token *cur, char *str) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    cur->next = tok;
    return tok;
}

// tokenize input string and return it
Token *tokenize(char *p) {
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        // skip whitespace
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (*p == '+' || *p == '-') {
            cur = new_token(TK_RESERVED, cur, p++);
            continue;
        }

        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        error("cannot tokenize");
    }

    new_token(TK_EOF, cur, p);
    return head.next;
}