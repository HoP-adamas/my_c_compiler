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

    for (Function *fn = prog; fn; fn = fn->next) {
        int offset = 0;
        for (Var *var = prog->locals; var; var = var->next) {
            offset += 8;
            var->offset = offset;
        }
        fn->stack_size = offset;
    }
    codegen(prog);
    return 0;
}
