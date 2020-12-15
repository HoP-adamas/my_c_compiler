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
bool consume(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len|| memcmp(token->str, op, token->len)) {
        return false;
    }
    token = token->next;
    return true;
}

// If the next token is the symbol we expect,
// consumes one token else reports an error.
void expect(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len)) {
        error("next token is expected '%c'", op);
    }
    token = token->next;
}

// If the next token  is a number,
// consumes one token and return this number else reports an error
int expect_number(void) {
    if (token->kind != TK_NUM) {
        error("next token is expected a number");
    }
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof(void) {
    return token->kind == TK_EOF;
}

// creates a new token and connects as the next token of current token
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

bool startswith(char *p, char *q) {
    return memcmp(p, q, strlen(q)) == 0;
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
        // Multi-letter punctuator
        if (startswith(p, "==") || startswith(p, "!=") || startswith(p, "<=") || startswith(p, ">=")) {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }
        // Single-letter punctuator
        if (strchr("+-*/()<>", *p)) {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        // Integer literal
        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            char *q = p;    // remain the head of Integer literal
            cur->val = strtol(p, &p, 10);
            cur->len = p - q;   // length of Integer
            continue;
        }

        error("cannot tokenize");
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}