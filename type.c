#include "9cc.h"

Type *int_type(void) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_INT;
    return ty;
}