#include "9cc.h"


int main(int argc, char **argv) {

    if (argc != 2) {
        error("%s: invalid number of arguments", argv[0]);
    }
    locals = NULL;
    // tokenize and parse input
    user_input = argv[1];
    token = tokenize(user_input);
    Function *prog = program();
    add_type(prog);

    for (Function *fn = prog; fn; fn = fn->next) {
        int offset = 0;
        for (VarList *vl = fn->locals; vl; vl = vl->next) {
            Var *var = vl->var;
            offset += size_of(var->ty);
            var->offset = offset;
        }
        fn->stack_size = offset;
    }
    codegen(prog);
    return 0;
}
