# include "9cc.h"

char *filename;

char *strndup(char *str, int chars)
{
    char *buffer;
    int n;

    buffer = (char *) malloc(chars +1);
    if (buffer)
    {
        for (n = 0; ((n < chars) && (str[n] != 0)) ; n++) buffer[n] = str[n];
        buffer[n] = 0;
    }

    return buffer;
}

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

    fprintf(stderr, "%s\n", user_input);

    char *line = loc;
    while(user_input < line && line[-1] != '\n') {
        line--;
    }

    char *end = loc;
    while (*end != '\n') {
        end++;
    }
    int line_num = 1;
    for (char *p = user_input; p < line; p++) {
        if (*p == '\n') {
            line_num++;
        }

    }
    int indent = fprintf(stderr, "%s:%d: ", filename, line_num);
    fprintf(stderr, "%.*s\n", (int)(end-line), line);
    int pos = loc - line + indent;
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

// Reports an error location and exit.
void error_tok(Token *tok, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  if (tok)
    verror_at(tok->str, fmt, ap);

  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// bool startswith(char *p, char *q) {
//     return memcmp(p, q, strlen(q)) == 0;
// }

// bool is_alpha(char c) {
//     return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
// }

// bool is_alnum(char c) {
//     return is_alpha(c) || ('0' <= c && c <= '9');
// }

Vector *new_vec(void) {
    Vector *v = malloc(sizeof(Vector));
    v->data = malloc(sizeof(void *) * 16);
    v->capacity = 16;
    v->len = 0;
    return v;
}

void vec_push(Vector *v, void *elem) {
    if (v->len == v->capacity) {
        v->capacity *= 2;
        v->data = realloc(v->data, sizeof(void *) * v->capacity);
    }
    v->data[v->len++] = elem;
}

Map *new_map(void) {
    Map *map = malloc(sizeof(Map));
    map->keys = new_vec();
    map->vals = new_vec();
    return map;
}

void map_put(Map *map, char *key, void *val) {
    vec_push(map->keys, key);
    vec_push(map->vals, val);
}

void *map_get(Map *map, char *key) {
    for (int i = map->keys->len - 1; i >= 0; i--) {
        if (!strcmp(map->keys->data[i], key)) {
            return map->vals->data[i];
        }
    }
    return NULL;
}