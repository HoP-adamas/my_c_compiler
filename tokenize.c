#include "9cc.h"

// the token we focus on
Token *token;

char *user_input;

// reports an error and exit.
// same args of printf()
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

static void verror_at(char *loc, char *fmt, va_list ap) {

    int pos = loc - user_input;

    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}
// Reports an error location and exit.
void error_at(char *loc, char *fmt, ...) {

    va_list ap;
    va_start(ap, fmt);
    verror_at(loc, fmt, ap);
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

// consume the current token if it is identifier
Token *consume_ident(void) {
    if (token->kind != TK_IDENT) {
        return NULL;
    }
    Token *t = token;
    token = token->next;
    return t;
}

// If the next token is the symbol we expect,
// consumes one token else reports an error.
void expect(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len)) {
        error_at(token->str, "next token is expected '%c'", op);
    }
    token = token->next;
}

// If the next token  is a number,
// consumes one token and return this number else reports an error
int expect_number(void) {
    if (token->kind != TK_NUM) {
        error_at(token->str, "next token is expected a number");
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

bool is_alpha(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

bool is_alnum(char c) {
    return is_alpha(c) || ('0' <= c && c <= '9');
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
        if (strchr("+-*/()<>=", *p)) {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        // return token
        if (strncmp(p, "return", 6) == 0 && !is_alnum(p[6])) {
            continue;
        }

        // Identifier multi letter
        // rule 1: the first letter of an identifier does not a number
        // rule 2: "_" is treated as an alphabet
        // rule 3: after second letter, we can use alphabet and number

        // check whether the first letter is alphabet
        if (is_alpha(*p)) {
            char *q = p++;
            while (is_alnum(*p)) {
                p++;
            }
            cur = new_token(TK_IDENT, cur, q, p-q);
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

        error_at(p, "invalid token");
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}