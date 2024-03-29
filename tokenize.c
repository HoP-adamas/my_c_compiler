#include "9cc.h"

// the token we focus on
Token *token;

char *filename;
char *user_input;


// Returns true if the current token matches a given string.
Token *peek(char *s) {
  if (token->kind != TK_RESERVED || strlen(s) != token->len ||
      memcmp(token->str, s, token->len))
    return NULL;
  return token;
}

// Consumes the current token if it matches a given string.
Token *consume(char *s) {
  if (!peek(s))
    return NULL;
  Token *t = token;
  token = token->next;
  return t;
}

Token *consume_ident(void) {
  if (token->kind != TK_IDENT)
    return NULL;
  Token *t = token;
  token = token->next;
  return t;
}

// If the next token is the symbol we expect,
// consumes one token else reports an error.
void expect(char *op) {
    if (!peek(op)) {
        error_tok(token, "next token is expected '%c'", op);
    }
    token = token->next;
}

// If the next token  is a number,
// consumes one token and return this number else reports an error
int expect_number(void) {
    if (token->kind != TK_NUM) {
        error_tok(token, "next token is expected a number");
    }
    int val = token->val;
    token = token->next;
    return val;
}

char *expect_ident(void) {
    if (token->kind != TK_IDENT) {
        error_tok(token, "expected an identifier");
    }
    char *s = strndup(token->str, token->len);
    token = token->next;
    return s;
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

char *starts_with_reserved(char *p) {
    // keyword
    static char *kw[] = {"return", "if", "else", "while", "for", "short", "int", "long", "sizeof", "char", "struct", "typedef"};

    for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
        int len = strlen(kw[i]);
        if (startswith(p, kw[i]) && !is_alnum(p[len])) {
            return kw[i];
        }
    }

        // Multi-letter punctuator
    static char *ops[] = {"==", "!=", "<=", ">=", "->"};

    for (int i = 0; i < sizeof(ops) / sizeof(*ops); i++) {
        if (startswith(p, ops[i])) {
            return ops[i];
        }
    }

    return NULL;
}

char get_escape_char(char c) {
    switch (c)
    {
    case 'a': return '\a';
    case 'b': return '\b';
    case 't': return '\t';
    case 'n': return '\n';
    case 'v': return '\v';
    case 'f': return '\f';
    case 'r': return '\r';
    case 'e': return 27;
    case '0': return 0;
    default: return c;
    }
}

Token *read_string_literal(Token *cur, char *start){
    char *p = start + 1;
    char buf[1024];
    int len = 0;

    for (;;) {
        if (len == sizeof(buf)) {
            error_at(start, "string literal too large");
        }
        if (*p == '\0') {
            error_at(start, "unclosed string literal");
        }
        if (*p == '"') {
            break;
        }

        if (*p == '\\') {
            p++;
            buf[len++] = get_escape_char(*p++);
        } else {
            buf[len++] = *p++;
        }
    }

    Token *tok = new_token(TK_STR, cur, start, p - start + 1);
    tok->contents = malloc(len + 1);
    memcpy(tok->contents, buf, len);
    tok->contents[len] = '\0';
    tok->cont_len = len + 1;
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
        // skip comment
        if (strncmp(p, "//", 2) == 0) {
            p += 2;
            while (*p != '\n') {
                p++;
            }
            continue;
        }
        // skip block comment
        if (strncmp(p, "/*", 2) == 0) {
            char *q = strstr(p + 2, "*/");
            if (!q) {
                error_at(p, "comment is not closd");
            }
            p = q + 2;
            continue;
        }

        char *kw = starts_with_reserved(p);
        if (kw) {
            int len = strlen(kw);
            cur = new_token(TK_RESERVED, cur, p, len);
            p += len;
            continue;
        }

        // Single-letter punctuator
        if (strchr("+-*/()<>=;{},&[].", *p)) {
            cur = new_token(TK_RESERVED, cur, p++, 1);
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
            cur = new_token(TK_IDENT, cur, q, p - q);
            continue;
        }

        // String literal
        if (*p == '"') {
            cur = read_string_literal(cur, p);
            p += cur->len;
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