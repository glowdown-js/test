#include "ufo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <libgen.h>
    #include <unistd.h>
    #include <limits.h>
#endif

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(size + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    return buf;
}

static void print_usage(const char *prog) {
    fprintf(stderr, "ufo %s\n", ufo_version());
    fprintf(stderr, "Usage: %s <input.fs> [-o output] [-c]\n", prog);
    fprintf(stderr, "       %s -v\n", prog);
    fprintf(stderr, "  -o <file>   output executable\n");
    fprintf(stderr, "  -c          keep generated C file (ufo_tmp.c)\n");
    fprintf(stderr, "  -v          show version\n");
}

static void get_exe_dir(const char *argv0, char *out, size_t out_size) {
#if defined(_WIN32)
    strncpy(out, argv0, out_size - 1);
    out[out_size - 1] = '\0';
    char *last = strrchr(out, '\\');
    if (!last) last = strrchr(out, '/');
    if (last) *(last + 1) = '\0';
    else out[0] = '\0';
#else
    char resolved[PATH_MAX];

    if (argv0[0] == '/') {
        strncpy(resolved, argv0, sizeof(resolved) - 1);
        resolved[sizeof(resolved) - 1] = '\0';
    } else if (strchr(argv0, '/')) {
        if (realpath(argv0, resolved) == NULL) {
            strncpy(resolved, argv0, sizeof(resolved) - 1);
            resolved[sizeof(resolved) - 1] = '\0';
        }
    } else {
        char *path_env = getenv("PATH");
        resolved[0] = '\0';
        if (path_env) {
            char *path_copy = strdup(path_env);
            char *dir = strtok(path_copy, ":");
            while (dir) {
                char candidate[PATH_MAX];
                snprintf(candidate, sizeof(candidate), "%s/%s", dir, argv0);
                if (access(candidate, X_OK) == 0) {
                    strncpy(resolved, candidate, sizeof(resolved) - 1);
                    resolved[sizeof(resolved) - 1] = '\0';
                    break;
                }
                dir = strtok(NULL, ":");
            }
            free(path_copy);
        }
        if (resolved[0] == '\0') {
            strncpy(resolved, argv0, sizeof(resolved) - 1);
            resolved[sizeof(resolved) - 1] = '\0';
        }
    }

    char *copy = strdup(resolved);
    if (!copy) { out[0] = '\0'; return; }
    char *dir = dirname(copy);
    snprintf(out, out_size, "%s/", dir);
    free(copy);
#endif
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-v") == 0) {
        printf("ufo %s\n", ufo_version());
        return 0;
    }

    char ufo_dir[4096];
    get_exe_dir(argv[0], ufo_dir, sizeof(ufo_dir));

    const char *input = NULL;
    const char *output = NULL;
    int keep_c = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output = argv[++i];
        } else if (strcmp(argv[i], "-c") == 0) {
            keep_c = 1;
        } else if (argv[i][0] != '-') {
            input = argv[i];
        }
    }

    if (!input) {
        fprintf(stderr, "ufo: no input file\n");
        print_usage(argv[0]);
        return 1;
    }

#if defined(_WIN32)
    if (!output) output = "hello.exe";
#else
    if (!output) output = "hello";
#endif

    char *src = read_file(input);
    if (!src) {
        fprintf(stderr, "ufo: cannot open %s\n", input);
        return 1;
    }

    int tok_count = 0;
    ufo_token_t *tokens = ufo_lex(src, strlen(src), &tok_count);
    if (!tokens) {
        fprintf(stderr, "ufo: out of memory\n");
        free(src);
        return 1;
    }

    ufo_compile_to_c(tokens, tok_count, "ufo_tmp.c");

    char cmd[8192];

#if defined(_WIN32)
    snprintf(cmd, sizeof(cmd),
             "\"%stcc.exe\" ufo_tmp.c -o %s",
             ufo_dir, output);
#elif defined(__APPLE__)
    char tcc_path[4096];
    snprintf(tcc_path, sizeof(tcc_path), "%stcc", ufo_dir);

    if (access(tcc_path, X_OK) == 0) {
        snprintf(cmd, sizeof(cmd),
                 "\"%s\" ufo_tmp.c -o %s",
                 tcc_path, output);
    } else {
        snprintf(cmd, sizeof(cmd),
                 "clang ufo_tmp.c -o %s",
                 output);
    }
#else
    snprintf(cmd, sizeof(cmd),
             "\"%stcc\" ufo_tmp.c -o %s",
             ufo_dir, output);
#endif

    int rc = system(cmd);

    if (!keep_c) {
        remove("ufo_tmp.c");
    }

    free(src);
    ufo_lex_free(tokens, tok_count);

    if (rc != 0) {
        fprintf(stderr, "ufo: tcc failed\n");
        return 1;
    }

    printf("ufo: compiled %s -> %s\n", input, output);
    return 0;
}