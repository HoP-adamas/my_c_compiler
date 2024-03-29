#include "9cc.h"

char *read_file(char *path) {
    // open and read the file.
    FILE *fp = fopen(path, "r");
    if (!fp) {
        error("cannot open %s: %s", path, strerror(errno));
    }

    int filemax = 10*1024*1024;
    char *buf = malloc(filemax);
    int size = fread(buf, 1, filemax-2, fp);
    if (!feof(fp)) {
        error("%s: file too large");
    }

    // Make sure that the string ends and with "\n\0".
    if (size == 0|| buf[size-1] != '\n') {
        buf[size++] = '\n';
    }
    buf[size] = '\0';
    return buf;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        error("%s: invalid number of arguments", argv[0]);
    }
    locals = NULL;
    // tokenize and parse input
    filename = argv[1];
    user_input = read_file(argv[1]);
    token = tokenize(user_input);
    Program *prog = program();
    add_type(prog);

    for (Function *fn = prog->fns; fn; fn = fn->next) {
        int offset = 0;
        for (VarList *vl = fn->locals; vl; vl = vl->next) {
            Var *var = vl->var;
            offset = align_to(offset, var->ty->align);
            offset += size_of(var->ty);
            var->offset = offset;
        }
        fn->stack_size = align_to(offset, 8);
    }

    codegen(prog);

    return 0;
}
