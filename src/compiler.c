#include "ufo.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

typedef struct {
    char name[64];
    cell_t value;
    int is_const;
} VarEntry;

typedef struct {
    ufo_token_t *tokens;
    int          count;
    int          pos;
    FILE        *out;
    int          begin_stack[256];
    int          begin_top;
    int          do_stack[256];
    int          do_top;
    int          while_stack[256];
    int          while_top;
    int          case_stack[256];
    int          case_top;
    int          label_count;
    char       **strings;
    int          nstrings;
    int          cap_strings;
    VarEntry    *vars;
    int          nvars;
    int          cap_vars;
    VarEntry    *consts;
    int          nconsts;
    int          cap_consts;
} Compiler;

static void emit_safe_name(FILE *out, const char *name) {
    for (const char *p = name; *p; p++) {
        if ((*p >= 'a' && *p <= 'z') ||
            (*p >= 'A' && *p <= 'Z') ||
            (*p >= '0' && *p <= '9')) {
            fputc(*p, out);
        } else {
            fputc('_', out);
        }
    }
}

static ufo_token_t *peek(Compiler *c) {
    if (c->pos >= c->count) return &c->tokens[c->count - 1];
    return &c->tokens[c->pos];
}

static ufo_token_t *advance(Compiler *c) {
    ufo_token_t *t = peek(c);
    if (c->pos < c->count) c->pos++;
    return t;
}

static int check(Compiler *c, ufo_tok_type_t t) {
    return peek(c)->type == t;
}

static int match(Compiler *c, ufo_tok_type_t t) {
    if (check(c, t)) { advance(c); return 1; }
    return 0;
}

static void emit(Compiler *c, const char *s) {
    fprintf(c->out, "%s\n", s);
}

static int new_label(Compiler *c) {
    return c->label_count++;
}

static int add_string(Compiler *c, const char *s, size_t len) {
    for (int i = 0; i < c->nstrings; i++) {
        if (strlen(c->strings[i]) == len &&
            memcmp(c->strings[i], s, len) == 0) {
            return i;
        }
    }
    if (c->nstrings >= c->cap_strings) {
        int new_cap = c->cap_strings ? c->cap_strings * 2 : 64;
        char **tmp = realloc(c->strings, new_cap * sizeof(char *));
        if (!tmp) return -1;
        c->strings = tmp;
        c->cap_strings = new_cap;
    }
    int idx = c->nstrings++;
    c->strings[idx] = malloc(len + 1);
    memcpy(c->strings[idx], s, len);
    c->strings[idx][len] = '\0';
    return idx;
}

static int is_variable(Compiler *c, const char *name) {
    for (int i = 0; i < c->nvars; i++) {
        if (strcmp(c->vars[i].name, name) == 0) return 1;
    }
    return 0;
}

static int is_constant(Compiler *c, const char *name) {
    for (int i = 0; i < c->nconsts; i++) {
        if (strcmp(c->consts[i].name, name) == 0) return 1;
    }
    return 0;
}

static cell_t get_constant(Compiler *c, const char *name) {
    for (int i = 0; i < c->nconsts; i++) {
        if (strcmp(c->consts[i].name, name) == 0) return c->consts[i].value;
    }
    return 0;
}

static void add_variable(Compiler *c, const char *name) {
    if (c->nvars >= c->cap_vars) {
        int new_cap = c->cap_vars ? c->cap_vars * 2 : 64;
        VarEntry *tmp = realloc(c->vars, new_cap * sizeof(VarEntry));
        if (!tmp) return;
        c->vars = tmp;
        c->cap_vars = new_cap;
    }
    strncpy(c->vars[c->nvars].name, name, 63);
    c->vars[c->nvars].name[63] = '\0';
    c->vars[c->nvars].value = 0;
    c->vars[c->nvars].is_const = 0;
    c->nvars++;
}

static void add_constant(Compiler *c, const char *name, cell_t value) {
    if (c->nconsts >= c->cap_consts) {
        int new_cap = c->cap_consts ? c->cap_consts * 2 : 64;
        VarEntry *tmp = realloc(c->consts, new_cap * sizeof(VarEntry));
        if (!tmp) return;
        c->consts = tmp;
        c->cap_consts = new_cap;
    }
    strncpy(c->consts[c->nconsts].name, name, 63);
    c->consts[c->nconsts].name[63] = '\0';
    c->consts[c->nconsts].value = value;
    c->consts[c->nconsts].is_const = 1;
    c->nconsts++;
}

static void compile_expr(Compiler *c);
static void compile_body(Compiler *c, ufo_tok_type_t s1, ufo_tok_type_t s2, ufo_tok_type_t s3);
static void compile_if(Compiler *c);
static void compile_begin(Compiler *c);
static void compile_until(Compiler *c);
static void compile_while(Compiler *c);
static void compile_do(Compiler *c);
static void compile_loop(Compiler *c);
static void compile_case(Compiler *c);

static void compile_expr(Compiler *c) {
    ufo_token_t *t = peek(c);

    switch (t->type) {
        case TOK_NUMBER: advance(c); fprintf(c->out, "push(%lldLL);\n", (long long)t->num); break;
        case TOK_DUP:    advance(c); emit(c, "push(peek());"); break;
        case TOK_DROP:   advance(c); emit(c, "pop();"); break;
        case TOK_SWAP:   advance(c); emit(c, "{cell_t b=pop();cell_t a=pop();push(b);push(a);}"); break;
        case TOK_OVER:   advance(c); emit(c, "{cell_t b=pop();cell_t a=pop();push(a);push(b);push(a);}"); break;
        case TOK_ROT:    advance(c); emit(c, "{cell_t c3=pop();cell_t b3=pop();cell_t a3=pop();push(b3);push(c3);push(a3);}"); break;
        case TOK_2DUP:   advance(c); emit(c, "{cell_t b=stack[sp-1];cell_t a=stack[sp-2];push(a);push(b);}"); break;
        case TOK_2DROP:  advance(c); emit(c, "pop();pop();"); break;
        case TOK_2SWAP:  advance(c); emit(c, "{cell_t d=pop();cell_t c3=pop();cell_t b3=pop();cell_t a3=pop();push(c3);push(d);push(a3);push(b3);}"); break;
        case TOK_2OVER:  advance(c); emit(c, "{cell_t d=stack[sp-1];cell_t c3=stack[sp-2];cell_t b3=stack[sp-3];cell_t a3=stack[sp-4];push(a3);push(b3);push(c3);push(d);push(a3);push(b3);}"); break;
        case TOK_NIP:    advance(c); emit(c, "{cell_t b=pop();pop();push(b);}"); break;
        case TOK_TUCK:   advance(c); emit(c, "{cell_t b=pop();cell_t a=pop();push(b);push(a);push(b);}"); break;
        case TOK_QDUP:   advance(c); emit(c, "{cell_t a=stack[sp-1];if(a!=0)push(a);}"); break;
        case TOK_DEPTH:  advance(c); emit(c, "push((cell_t)sp);"); break;
        case TOK_PICK:   advance(c); emit(c, "{cell_t n=pop();push(stack[sp-1-n]);}"); break;
        case TOK_TO_R:   advance(c); emit(c, "{cell_t a=pop();rpush(a);}"); break;
        case TOK_R_FROM: advance(c); emit(c, "push(rpop());"); break;
        case TOK_R_FETCH: advance(c); emit(c, "push(rpeek());"); break;
        case TOK_PLUS:   advance(c); emit(c, "{cell_t b=pop();cell_t a=pop();push(a+b);}"); break;
        case TOK_MINUS:  advance(c); emit(c, "{cell_t b=pop();cell_t a=pop();push(a-b);}"); break;
        case TOK_STAR:   advance(c); emit(c, "{cell_t b=pop();cell_t a=pop();push(a*b);}"); break;
        case TOK_SLASH:  advance(c); emit(c, "{cell_t b=pop();cell_t a=pop();push(b?a/b:0);}"); break;
        case TOK_MOD:    advance(c); emit(c, "{cell_t b=pop();cell_t a=pop();push(b?a%b:0);}"); break;
        case TOK_SLASH_MOD: advance(c); emit(c, "{cell_t b=pop();cell_t a=pop();push(b?a%b:0);push(b?a/b:0);}"); break;
        case TOK_STAR_SLASH: advance(c); emit(c, "{cell_t c3=pop();cell_t b3=pop();cell_t a3=pop();push(c3?((long long)a3*b3)/c3:0);}"); break;
        case TOK_STAR_SLASH_MOD: advance(c); emit(c, "{cell_t c3=pop();cell_t b3=pop();cell_t a3=pop();long long p3=(long long)a3*b3;push(c3?p3%c3:0);push(c3?p3/c3:0);}"); break;
        case TOK_ABS:    advance(c); emit(c, "{cell_t a=pop();push(a<0?-a:a);}"); break;
        case TOK_NEGATE: advance(c); emit(c, "{cell_t a=pop();push(-a);}"); break;
        case TOK_MIN:    advance(c); emit(c, "{cell_t b=pop();cell_t a=pop();push(a<b?a:b);}"); break;
        case TOK_MAXWORD: advance(c); emit(c, "{cell_t b=pop();cell_t a=pop();push(a>b?a:b);}"); break;
        case TOK_1PLUS:  advance(c); emit(c, "{cell_t a=pop();push(a+1);}"); break;
        case TOK_1MINUS: advance(c); emit(c, "{cell_t a=pop();push(a-1);}"); break;
        case TOK_2PLUS:  advance(c); emit(c, "{cell_t a=pop();push(a+2);}"); break;
        case TOK_2MINUS: advance(c); emit(c, "{cell_t a=pop();push(a-2);}"); break;
        case TOK_2STAR:  advance(c); emit(c, "{cell_t a=pop();push(a*2);}"); break;
        case TOK_2SLASH: advance(c); emit(c, "{cell_t a=pop();push(a/2);}"); break;
        case TOK_EQ:     advance(c); emit(c, "{cell_t b=pop();cell_t a=pop();push(a==b);}"); break;
        case TOK_NE:     advance(c); emit(c, "{cell_t b=pop();cell_t a=pop();push(a!=b);}"); break;
        case TOK_LT:     advance(c); emit(c, "{cell_t b=pop();cell_t a=pop();push(a<b);}"); break;
        case TOK_GT:     advance(c); emit(c, "{cell_t b=pop();cell_t a=pop();push(a>b);}"); break;
        case TOK_LE:     advance(c); emit(c, "{cell_t b=pop();cell_t a=pop();push(a<=b);}"); break;
        case TOK_GE:     advance(c); emit(c, "{cell_t b=pop();cell_t a=pop();push(a>=b);}"); break;
        case TOK_ZERO_EQ: advance(c); emit(c, "{cell_t a=pop();push(a==0);}"); break;
        case TOK_ZERO_LT: advance(c); emit(c, "{cell_t a=pop();push(a<0);}"); break;
        case TOK_ZERO_GT: advance(c); emit(c, "{cell_t a=pop();push(a>0);}"); break;
        case TOK_AND:    advance(c); emit(c, "{cell_t b=pop();cell_t a=pop();push(a&b);}"); break;
        case TOK_OR:     advance(c); emit(c, "{cell_t b=pop();cell_t a=pop();push(a|b);}"); break;
        case TOK_XOR:    advance(c); emit(c, "{cell_t b=pop();cell_t a=pop();push(a^b);}"); break;
        case TOK_INVERT: advance(c); emit(c, "{cell_t a=pop();push(~a);}"); break;
        case TOK_LSHIFT: advance(c); emit(c, "{cell_t b=pop();cell_t a=pop();push(a<<b);}"); break;
        case TOK_RSHIFT: advance(c); emit(c, "{cell_t b=pop();cell_t a=pop();push(a>>b);}"); break;
        case TOK_LOAD:   advance(c); emit(c, "push(*(cell_t*)(size_t)pop());"); break;
        case TOK_STORE:  advance(c); emit(c, "{cell_t addr=pop();cell_t v=pop();*(cell_t*)(size_t)addr=v;}"); break;
        case TOK_CFETCH: advance(c); emit(c, "push((cell_t)*(unsigned char*)(size_t)pop());"); break;
        case TOK_CSTORE: advance(c); emit(c, "{cell_t addr=pop();cell_t v=pop();*(unsigned char*)(size_t)addr=(unsigned char)v;}"); break;
        case TOK_ADD_STORE: advance(c); emit(c, "{cell_t addr=pop();cell_t v=pop();*(cell_t*)(size_t)addr+=v;}"); break;
        case TOK_2FETCH: advance(c); emit(c, "{cell_t a=pop();push(*(cell_t*)(size_t)a);push(*(cell_t*)(size_t)(a+sizeof(cell_t)));}"); break;
        case TOK_2STORE: advance(c); emit(c, "{cell_t addr=pop();cell_t v2=pop();cell_t v1=pop();*(cell_t*)(size_t)addr=v1;*(cell_t*)(size_t)(addr+sizeof(cell_t))=v2;}"); break;
        case TOK_CELLPLUS: advance(c); emit(c, "{cell_t a=pop();push(a+sizeof(cell_t));}"); break;
        case TOK_CELLS:  advance(c); emit(c, "{cell_t a=pop();push(a*sizeof(cell_t));}"); break;
        case TOK_CHARPLUS: advance(c); emit(c, "{cell_t a=pop();push(a+1);}"); break;
        case TOK_CHARS:  advance(c); emit(c, "{cell_t a=pop();push(a);}"); break;
        case TOK_ALIGN:  advance(c); emit(c, "/* align */"); break;
        case TOK_ALIGNED: advance(c); emit(c, "{cell_t a=pop();push((a+7)&~7);}"); break;
        case TOK_PAD:    advance(c); emit(c, "push((cell_t)pad_buf);"); break;
        case TOK_HERE:   advance(c); emit(c, "push((cell_t)heap_top);"); break;
        case TOK_ALLOT:  advance(c); emit(c, "{cell_t n=pop();heap_top+=n;}"); break;

        case TOK_COUNT:
            advance(c);
            emit(c, "{cell_t a=pop();push(a+1);push((cell_t)*(unsigned char*)(size_t)a);}");
            break;

        case TOK_CMOVE:
            advance(c);
            emit(c, "{cell_t n=pop();cell_t src=pop();cell_t dst=pop();for(cell_t i=0;i<n;i++)*(unsigned char*)(size_t)(dst+i)=*(unsigned char*)(size_t)(src+i);}");
            break;

        case TOK_CMOVE_UP:
            advance(c);
            emit(c, "{cell_t n=pop();cell_t src=pop();cell_t dst=pop();for(cell_t i=n;i>0;i--)*(unsigned char*)(size_t)(dst+i-1)=*(unsigned char*)(size_t)(src+i-1);}");
            break;

        case TOK_MOVE:
            advance(c);
            emit(c, "{cell_t n=pop();cell_t src=pop();cell_t dst=pop();memmove((void*)(size_t)dst,(void*)(size_t)src,(size_t)n);}");
            break;

        case TOK_FILL:
            advance(c);
            emit(c, "{cell_t ch=pop();cell_t n=pop();cell_t addr=pop();memset((void*)(size_t)addr,(int)ch,(size_t)n);}");
            break;

        case TOK_ERASE:
            advance(c);
            emit(c, "{cell_t n=pop();cell_t addr=pop();memset((void*)(size_t)addr,0,(size_t)n);}");
            break;

        case TOK_BLANK:
            advance(c);
            emit(c, "{cell_t n=pop();cell_t addr=pop();memset((void*)(size_t)addr,' ',(size_t)n);}");
            break;

        case TOK_COMPARE:
            advance(c);
            emit(c, "{cell_t n2=pop();cell_t a2=pop();cell_t n1=pop();cell_t a1=pop();int r=memcmp((void*)(size_t)a1,(void*)(size_t)a2,(size_t)(n1<n2?n1:n2));if(r==0){if(n1<n2)r=-1;else if(n1>n2)r=1;}push((cell_t)r);}");
            break;

        case TOK_SEARCH:
            advance(c);
            emit(c, "{cell_t n2=pop();cell_t a2=pop();cell_t n1=pop();cell_t a1=pop();cell_t found=-1;for(cell_t i=0;i<=n1-n2;i++){if(memcmp((void*)(size_t)(a1+i),(void*)(size_t)a2,(size_t)n2)==0){found=a1+i;break;}}if(found>=0){push(found);push(n1-(found-a1));push(1);}else{push(a1);push(n1);push(0);}}");
            break;

        case TOK_MINUS_TRAILING:
            advance(c);
            emit(c, "{cell_t n=pop();cell_t a=pop();while(n>0&&*(unsigned char*)(size_t)(a+n-1)==' ')n--;push(a);push(n);}");
            break;

        case TOK_SLASH_STRING:
            advance(c);
            emit(c, "{cell_t n=pop();cell_t a=pop();cell_t k=pop();push(a+k);push(n-k);}");
            break;

        case TOK_BL:
            advance(c);
            emit(c, "push((cell_t)32);");
            break;

        case TOK_CHAR:
            advance(c);
            if (check(c, TOK_WORD)) {
                ufo_token_t *name = advance(c);
                fprintf(c->out, "push((cell_t)%d);\n", (unsigned char)name->text[0]);
            }
            break;

        case TOK_BRACKET_CHAR:
            advance(c);
            if (check(c, TOK_WORD)) {
                ufo_token_t *name = advance(c);
                fprintf(c->out, "push((cell_t)%d);\n", (unsigned char)name->text[0]);
            }
            break;

        case TOK_I:
            advance(c);
            if (c->do_top >= 2) {
                int id = c->do_stack[c->do_top - 2];
                fprintf(c->out, "push(loop_index_%d);\n", id);
            }
            break;

        case TOK_J:
            advance(c);
            if (c->do_top >= 4) {
                int id = c->do_stack[c->do_top - 4];
                fprintf(c->out, "push(loop_index_%d);\n", id);
            }
            break;

        case TOK_K:
            advance(c);
            if (c->do_top >= 6) {
                int id = c->do_stack[c->do_top - 6];
                fprintf(c->out, "push(loop_index_%d);\n", id);
            }
            break;

        case TOK_LEAVE:
            advance(c);
            if (c->do_top >= 2) {
                fprintf(c->out, "goto L%d;\n", c->do_stack[c->do_top - 1]);
            }
            break;

        case TOK_RECURSE:
            advance(c);
            break;

        case TOK_AGAIN:
            if (c->begin_top > 0) {
                int begin_label = c->begin_stack[--c->begin_top];
                fprintf(c->out, "goto L%d;\n", begin_label);
            }
            advance(c);
            break;

        case TOK_DOT:    advance(c); emit(c, "printf(\"%lld \",(long long)pop());"); break;
        case TOK_U_DOT:  advance(c); emit(c, "printf(\"%llu \",(unsigned long long)pop());"); break;
        case TOK_H_DOT:  advance(c); emit(c, "printf(\"%llx \",(unsigned long long)pop());"); break;
        case TOK_EMIT:   advance(c); emit(c, "putchar((int)pop());"); break;
        case TOK_CR:     advance(c); emit(c, "putchar('\\n');"); break;
        case TOK_SPACE:  advance(c); emit(c, "putchar(' ');"); break;
        case TOK_SPACES: advance(c); emit(c, "{cell_t n=pop();while(n-->0)putchar(' ');}"); break;
        case TOK_TYPE:   advance(c); emit(c, "{cell_t n=pop();const char*s=(const char*)(size_t)pop();fwrite(s,1,n,stdout);}"); break;
        case TOK_KEY:    advance(c); emit(c, "push((cell_t)getchar());"); break;
        case TOK_KEY_Q:  advance(c); emit(c, "{int c=fgetc(stdin);if(c!=EOF){ungetc(c,stdin);push(-1);}else push(0);}"); break;
        case TOK_ACCEPT: advance(c); emit(c, "{cell_t max=pop();char*buf=(char*)(size_t)pop();if(fgets(buf,max,stdin)){size_t l=strlen(buf);if(l>0&&buf[l-1]=='\\n')buf[--l]=0;push((cell_t)l);}else push(0);}"); break;

        case TOK_RANDOM:
            advance(c);
            emit(c, "{if(!rand_seeded){srand((unsigned)time(NULL));rand_seeded=1;}cell_t rmax=pop();if(rmax<=0)rmax=100;push((cell_t)(rand()%rmax));}");
            break;

        case TOK_INPUT:
            advance(c);
            emit(c, "{char buf[64];if(fgets(buf,64,stdin))push((cell_t)atoll(buf));else push(0);}");
            break;

        case TOK_TO_NUMBER:
            advance(c);
            emit(c, "{cell_t len=pop();cell_t addr=pop();char buf[64];if(len>63)len=63;memcpy(buf,(void*)(size_t)addr,(size_t)len);buf[len]=0;push((cell_t)atoll(buf));}");
            break;

        case TOK_PARSE:
            advance(c);
            emit(c, "{char buf[256];if(fgets(buf,256,stdin)){size_t l=strlen(buf);if(l>0&&buf[l-1]=='\\n')buf[--l]=0;push((cell_t)l);}else push(0);}");
            break;

        case TOK_WORD_W:
            advance(c);
            emit(c, "{char buf[256];if(scanf(\"%255s\",buf)==1){size_t l=strlen(buf);if(l>0&&buf[l-1]=='\\n')buf[--l]=0;push((cell_t)l);}else push(0);}");
            break;

        case TOK_DOT_QUOTE:
            advance(c);
            if (check(c, TOK_STRING)) {
                ufo_token_t *s = advance(c);
                int idx = add_string(c, s->text, s->len);
                fprintf(c->out, "printf(\"%%s\", str%d);\n", idx);
            }
            break;

        case TOK_S_QUOTE:
            advance(c);
            if (check(c, TOK_STRING)) {
                ufo_token_t *s = advance(c);
                int idx = add_string(c, s->text, s->len);
                fprintf(c->out, "push((cell_t)str%d);\n", idx);
                fprintf(c->out, "push((cell_t)%zu);\n", s->len);
            }
            break;

        case TOK_VARIABLE:
            advance(c);
            if (check(c, TOK_WORD)) {
                ufo_token_t *name = advance(c);
                add_variable(c, name->text);
            }
            break;

        case TOK_CONSTANT:
            advance(c);
            if (check(c, TOK_WORD)) {
                ufo_token_t *name = advance(c);
                add_constant(c, name->text, 0);
            }
            break;

        case TOK_VALUE:
            advance(c);
            if (check(c, TOK_WORD)) {
                ufo_token_t *name = advance(c);
                add_variable(c, name->text);
            }
            break;

        case TOK_TO:
            advance(c);
            if (check(c, TOK_WORD)) {
                ufo_token_t *name = advance(c);
                fprintf(c->out, "{cell_t v=pop();var_");
                emit_safe_name(c->out, name->text);
                fprintf(c->out, "=v;}\n");
            }
            break;

        case TOK_CREATE:
            advance(c);
            if (check(c, TOK_WORD)) {
                ufo_token_t *name = advance(c);
                add_variable(c, name->text);
            }
            break;

        case TOK_EXIT:
            advance(c);
            emit(c, "return;");
            break;

        case TOK_WORD:
            if (is_constant(c, t->text)) {
                fprintf(c->out, "push(%lldLL);\n", (long long)get_constant(c, t->text));
            } else if (is_variable(c, t->text)) {
                fprintf(c->out, "push((cell_t)&var_");
                emit_safe_name(c->out, t->text);
                fprintf(c->out, ");\n");
            } else {
                fprintf(c->out, "fn_");
                emit_safe_name(c->out, t->text);
                fprintf(c->out, "();\n");
            }
            advance(c);
            break;

        default:
            advance(c);
            break;
    }
}

static void compile_body(Compiler *c, ufo_tok_type_t s1, ufo_tok_type_t s2, ufo_tok_type_t s3) {
    while (!check(c, s1) && !check(c, s2) && !check(c, s3) &&
           !check(c, TOK_EOF)) {
        if (check(c, TOK_IF)) { advance(c); compile_if(c); }
        else if (check(c, TOK_BEGIN)) { advance(c); compile_begin(c); }
        else if (check(c, TOK_UNTIL)) { compile_until(c); }
        else if (check(c, TOK_WHILE)) { compile_while(c); }
        else if (check(c, TOK_DO) || check(c, TOK_QDO)) { advance(c); compile_do(c); }
        else if (check(c, TOK_LOOP) || check(c, TOK_PLUSLOOP)) { compile_loop(c); }
        else if (check(c, TOK_CASE)) { advance(c); compile_case(c); }
        else { compile_expr(c); }
    }
}

static void compile_if(Compiler *c) {
    int else_label = new_label(c);
    int end_label = new_label(c);
    fprintf(c->out, "{cell_t cond=pop();if(cond==0){goto L%d;}}\n", else_label);

    compile_body(c, TOK_ELSE, TOK_THEN, TOK_EOF);

    if (match(c, TOK_ELSE)) {
        fprintf(c->out, "goto L%d;\n", end_label);
        fprintf(c->out, "L%d:;\n", else_label);
        compile_body(c, TOK_THEN, TOK_EOF, TOK_EOF);
    } else {
        fprintf(c->out, "L%d:;\n", else_label);
    }
    if (match(c, TOK_THEN)) {
        fprintf(c->out, "L%d:;\n", end_label);
    }
}

static void compile_begin(Compiler *c) {
    int begin_label = new_label(c);
    fprintf(c->out, "L%d:;\n", begin_label);
    c->begin_stack[c->begin_top++] = begin_label;
    compile_body(c, TOK_UNTIL, TOK_WHILE, TOK_AGAIN);
}

static void compile_until(Compiler *c) {
    if (c->begin_top > 0) {
        int begin_label = c->begin_stack[--c->begin_top];
        fprintf(c->out, "{cell_t cond=pop();if(cond==0){goto L%d;}}\n", begin_label);
    }
    advance(c);
}

static void compile_while(Compiler *c) {
    advance(c);
    int end_label = new_label(c);
    fprintf(c->out, "{cell_t cond=pop();if(cond==0){goto L%d;}}\n", end_label);
    c->while_stack[c->while_top++] = end_label;
    compile_body(c, TOK_REPEAT, TOK_EOF, TOK_EOF);
    if (match(c, TOK_REPEAT)) {
        if (c->begin_top > 0) {
            int begin_label = c->begin_stack[--c->begin_top];
            fprintf(c->out, "goto L%d;\n", begin_label);
        }
        fprintf(c->out, "L%d:;\n", end_label);
        c->while_top--;
    }
}

static void compile_do(Compiler *c) {
    int do_label = new_label(c);
    int end_label = new_label(c);
    c->do_stack[c->do_top++] = do_label;
    c->do_stack[c->do_top++] = end_label;

    int id = do_label;

    fprintf(c->out, "cell_t loop_index_%d = pop();\n", id);
    fprintf(c->out, "cell_t loop_limit_%d = pop();\n", id);
    fprintf(c->out, "L%d:;\n", do_label);
    fprintf(c->out, "if(loop_index_%d >= loop_limit_%d) goto L%d;\n",
            id, id, end_label);
}

static void compile_loop(Compiler *c) {
    if (c->do_top >= 2) {
        int end_label = c->do_stack[--c->do_top];
        int do_label = c->do_stack[--c->do_top];
        int id = do_label;

        fprintf(c->out, "loop_index_%d++;\n", id);
        fprintf(c->out, "goto L%d;\n", do_label);
        fprintf(c->out, "L%d:;\n", end_label);
    }
    advance(c);
}

static void compile_case(Compiler *c) {
    int end_label = new_label(c);

    fprintf(c->out, "{cell_t case_val = pop();\n");

    while (!check(c, TOK_ENDCASE) && !check(c, TOK_EOF)) {
        if (check(c, TOK_OF)) {
            advance(c);
            int match_label = new_label(c);
            int skip_label = new_label(c);
            fprintf(c->out, "if(case_val == (cell_t)pop()){goto L%d;}\n", match_label);
            fprintf(c->out, "goto L%d;\n", skip_label);
            fprintf(c->out, "L%d:;\n", match_label);
            compile_body(c, TOK_ENDOF, TOK_ENDCASE, TOK_EOF);
            fprintf(c->out, "goto L%d;\n", end_label);
            fprintf(c->out, "L%d:;\n", skip_label);
        } else {
            compile_expr(c);
        }
    }

    match(c, TOK_ENDCASE);
    fprintf(c->out, "L%d:;\n", end_label);
    fprintf(c->out, "}\n");
}

static void compile_colon(Compiler *c) {
    advance(c);
    if (check(c, TOK_WORD)) {
        ufo_token_t *name = advance(c);

        c->begin_top = 0;
        c->do_top = 0;
        c->while_top = 0;
        c->case_top = 0;

        fprintf(c->out, "void fn_");
        emit_safe_name(c->out, name->text);
        fprintf(c->out, "(void){\n");

        compile_body(c, TOK_SEMICOLON, TOK_EOF, TOK_EOF);

        if (match(c, TOK_SEMICOLON)) {
            fprintf(c->out, "}\n");
        }
    }
}

int ufo_compile_to_c(ufo_token_t *tokens, int count, const char *filename) {
    static Compiler c;
    memset(&c, 0, sizeof(c));
    c.tokens = tokens;
    c.count = count;
    c.pos = 0;
    c.strings = NULL;
    c.nstrings = 0;
    c.cap_strings = 0;
    c.vars = NULL;
    c.nvars = 0;
    c.cap_vars = 0;
    c.consts = NULL;
    c.nconsts = 0;
    c.cap_consts = 0;

    c.out = fopen(filename, "w");
    if (!c.out) return -1;

    for (int i = 0; i < count; i++) {
        if (tokens[i].type == TOK_STRING) {
            add_string(&c, tokens[i].text, tokens[i].len);
        }
    }

    fprintf(c.out, "#include <stdio.h>\n");
    fprintf(c.out, "#include <stdlib.h>\n");
    fprintf(c.out, "#include <stdint.h>\n");
    fprintf(c.out, "#include <string.h>\n");
    fprintf(c.out, "#include <time.h>\n");
    fprintf(c.out, "typedef int64_t cell_t;\n");
    fprintf(c.out, "#define STACK_SIZE 4096\n");
    fprintf(c.out, "static cell_t stack[STACK_SIZE];\n");
    fprintf(c.out, "static int sp = 0;\n");
    fprintf(c.out, "static cell_t rstack[STACK_SIZE];\n");
    fprintf(c.out, "static int rsp = 0;\n");
    fprintf(c.out, "static unsigned char heap[65536];\n");
    fprintf(c.out, "static cell_t heap_top = 0;\n");
    fprintf(c.out, "static char pad_buf[256];\n");
    fprintf(c.out, "static int rand_seeded = 0;\n");
    fprintf(c.out, "static void push(cell_t v){if(sp>=STACK_SIZE){fprintf(stderr,\"stack overflow\\n\");exit(1);}stack[sp++]=v;}\n");
    fprintf(c.out, "static cell_t pop(void){if(sp<=0){fprintf(stderr,\"stack underflow\\n\");exit(1);}return stack[--sp];}\n");
    fprintf(c.out, "static cell_t peek(void){return stack[sp-1];}\n");
    fprintf(c.out, "static void rpush(cell_t v){rstack[rsp++]=v;}\n");
    fprintf(c.out, "static cell_t rpop(void){return rstack[--rsp];}\n");
    fprintf(c.out, "static cell_t rpeek(void){return rstack[rsp-1];}\n");

    for (int i = 0; i < c.nstrings; i++) {
        fprintf(c.out, "static const char *str%d = ", i);
        fputc('"', c.out);
        for (const char *p = c.strings[i]; *p; p++) {
            if (*p == '\\') fprintf(c.out, "\\\\");
            else if (*p == '"') fprintf(c.out, "\\\"");
            else if (*p == '\n') fprintf(c.out, "\\n");
            else fputc(*p, c.out);
        }
        fprintf(c.out, "\";\n");
    }

    for (int i = 0; i < count; i++) {
        if ((tokens[i].type == TOK_VARIABLE ||
             tokens[i].type == TOK_VALUE ||
             tokens[i].type == TOK_CREATE) &&
            i + 1 < count &&
            tokens[i + 1].type == TOK_WORD) {
            fprintf(c.out, "static cell_t var_");
            emit_safe_name(c.out, tokens[i + 1].text);
            fprintf(c.out, " = 0;\n");
        }
    }

    for (int i = 0; i < count; i++) {
        if (tokens[i].type == TOK_COLON &&
            i + 1 < count &&
            tokens[i + 1].type == TOK_WORD) {
            fprintf(c.out, "void fn_");
            emit_safe_name(c.out, tokens[i + 1].text);
            fprintf(c.out, "(void);\n");
        }
    }

    fprintf(c.out, "int main(void){\n");

    while (!check(&c, TOK_EOF)) {
        if (check(&c, TOK_COLON)) {
            advance(&c);
            if (check(&c, TOK_WORD)) {
                advance(&c);
                while (!check(&c, TOK_SEMICOLON) && !check(&c, TOK_EOF)) advance(&c);
                match(&c, TOK_SEMICOLON);
            }
        } else {
            compile_expr(&c);
        }
    }

    fprintf(c.out, "return 0;\n");
    fprintf(c.out, "}\n\n");

    c.pos = 0;
    while (!check(&c, TOK_EOF)) {
        if (check(&c, TOK_COLON)) {
            compile_colon(&c);
        } else {
            advance(&c);
        }
    }

    fclose(c.out);

    for (int i = 0; i < c.nstrings; i++) free(c.strings[i]);
    free(c.strings);
    free(c.vars);
    free(c.consts);

    return 0;
}