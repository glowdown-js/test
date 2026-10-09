#include "ufo.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

static const struct {
    const char   *name;
    ufo_tok_type_t type;
} keywords[] = {
    { ":",          TOK_COLON },
    { ";",          TOK_SEMICOLON },
    { "IF",         TOK_IF },
    { "ELSE",       TOK_ELSE },
    { "THEN",       TOK_THEN },
    { "DO",         TOK_DO },
    { "LOOP",       TOK_LOOP },
    { "+LOOP",      TOK_PLUSLOOP },
    { "?DO",        TOK_QDO },
    { "BEGIN",      TOK_BEGIN },
    { "UNTIL",      TOK_UNTIL },
    { "WHILE",      TOK_WHILE },
    { "REPEAT",     TOK_REPEAT },
    { "AGAIN",      TOK_AGAIN },
    { "LEAVE",      TOK_LEAVE },
    { "UNLOOP",     TOK_UNLOOP },
    { "I",          TOK_I },
    { "J",          TOK_J },
    { "K",          TOK_K },
    { "CASE",       TOK_CASE },
    { "OF",         TOK_OF },
    { "ENDOF",      TOK_ENDOF },
    { "ENDCASE",    TOK_ENDCASE },
    { ".\"",        TOK_DOT_QUOTE },
    { "S\"",        TOK_S_QUOTE },
    { "VARIABLE",   TOK_VARIABLE },
    { "CONSTANT",   TOK_CONSTANT },
    { "VALUE",      TOK_VALUE },
    { "TO",         TOK_TO },
    { "CREATE",     TOK_CREATE },
    { "ALLOT",      TOK_ALLOT },
    { "HERE",       TOK_HERE },
    { "2@",         TOK_2FETCH },
    { "2!",         TOK_2STORE },
    { "CELL+",      TOK_CELLPLUS },
    { "CELLS",      TOK_CELLS },
    { "CHAR+",      TOK_CHARPLUS },
    { "CHARS",      TOK_CHARS },
    { "ALIGN",      TOK_ALIGN },
    { "ALIGNED",    TOK_ALIGNED },
    { "PAD",        TOK_PAD },
    { "COUNT",      TOK_COUNT },
    { "CMOVE",      TOK_CMOVE },
    { "CMOVE>",     TOK_CMOVE_UP },
    { "MOVE",       TOK_MOVE },
    { "FILL",       TOK_FILL },
    { "ERASE",      TOK_ERASE },
    { "BLANK",      TOK_BLANK },
    { "COMPARE",    TOK_COMPARE },
    { "SEARCH",     TOK_SEARCH },
    { "-TRAILING",  TOK_MINUS_TRAILING },
    { "/STRING",    TOK_SLASH_STRING },
    { "BL",         TOK_BL },
    { "CHAR",       TOK_CHAR },
    { "[CHAR]",     TOK_BRACKET_CHAR },
    { "IMMEDIATE",  TOK_IMMEDIATE },
    { "RECURSE",    TOK_RECURSE },
    { "EXIT",       TOK_EXIT },
    { "DUP",        TOK_DUP },
    { "DROP",       TOK_DROP },
    { "SWAP",       TOK_SWAP },
    { "OVER",       TOK_OVER },
    { "ROT",        TOK_ROT },
    { "2DUP",       TOK_2DUP },
    { "2DROP",      TOK_2DROP },
    { "2SWAP",      TOK_2SWAP },
    { "2OVER",      TOK_2OVER },
    { "NIP",        TOK_NIP },
    { "TUCK",       TOK_TUCK },
    { "?DUP",       TOK_QDUP },
    { "DEPTH",      TOK_DEPTH },
    { "PICK",       TOK_PICK },
    { ">R",         TOK_TO_R },
    { "R>",         TOK_R_FROM },
    { "R@",         TOK_R_FETCH },
    { "+",          TOK_PLUS },
    { "-",          TOK_MINUS },
    { "*",          TOK_STAR },
    { "/",          TOK_SLASH },
    { "MOD",        TOK_MOD },
    { "/MOD",       TOK_SLASH_MOD },
    { "*/",         TOK_STAR_SLASH },
    { "*/MOD",      TOK_STAR_SLASH_MOD },
    { "ABS",        TOK_ABS },
    { "NEGATE",     TOK_NEGATE },
    { "MIN",        TOK_MIN },
    { "MAX",        TOK_MAXWORD },
    { "1+",         TOK_1PLUS },
    { "1-",         TOK_1MINUS },
    { "2+",         TOK_2PLUS },
    { "2-",         TOK_2MINUS },
    { "2*",         TOK_2STAR },
    { "2/",         TOK_2SLASH },
    { "=",          TOK_EQ },
    { "<>",         TOK_NE },
    { "<",          TOK_LT },
    { ">",          TOK_GT },
    { "<=",         TOK_LE },
    { ">=",         TOK_GE },
    { "0=",         TOK_ZERO_EQ },
    { "0<",         TOK_ZERO_LT },
    { "0>",         TOK_ZERO_GT },
    { "AND",        TOK_AND },
    { "OR",         TOK_OR },
    { "XOR",        TOK_XOR },
    { "INVERT",     TOK_INVERT },
    { "LSHIFT",     TOK_LSHIFT },
    { "RSHIFT",     TOK_RSHIFT },
    { "EMIT",       TOK_EMIT },
    { "CR",         TOK_CR },
    { ".",          TOK_DOT },
    { ".S",         TOK_DOTS },
    { "TYPE",       TOK_TYPE },
    { "KEY",        TOK_KEY },
    { "KEY?",       TOK_KEY_Q },
    { "ACCEPT",     TOK_ACCEPT },
    { "PARSE",      TOK_PARSE },
    { "WORD",       TOK_WORD_W },
    { ">NUMBER",    TOK_TO_NUMBER },
    { "RANDOM",     TOK_RANDOM },
    { "INPUT",      TOK_INPUT },
    { "U.",         TOK_U_DOT },
    { "H.",         TOK_H_DOT },
    { "SPACE",      TOK_SPACE },
    { "SPACES",     TOK_SPACES },
    { "@",          TOK_LOAD },
    { "!",          TOK_STORE },
    { "C@",         TOK_CFETCH },
    { "C!",         TOK_CSTORE },
    { "+!",         TOK_ADD_STORE },
    { "FETCH",      TOK_FETCH },
    { NULL, 0 }
};

static ufo_tok_type_t lookup_keyword(const char *name, size_t len) {
    for (int i = 0; keywords[i].name; i++) {
        if (strlen(keywords[i].name) == len &&
            strncmp(keywords[i].name, name, len) == 0) {
            return keywords[i].type;
        }
    }
    return TOK_WORD;
}

static int is_number(const char *s, size_t len, cell_t *out) {
    if (len == 0) return 0;
    size_t i = 0;
    int neg = 0;
    if (s[0] == '-') { neg = 1; i = 1; }
    else if (s[0] == '+') { i = 1; }
    if (i >= len) return 0;

    cell_t v = 0;
    int base = 10;

    if (i + 1 < len && s[i] == '0' && (s[i+1] == 'x' || s[i+1] == 'X')) {
        base = 16;
        i += 2;
    } else if (i + 1 < len && s[i] == '0' && (s[i+1] == 'b' || s[i+1] == 'B')) {
        base = 2;
        i += 2;
    }

    if (i >= len) return 0;

    for (; i < len; i++) {
        int d;
        char c = s[i];
        if (c >= '0' && c <= '9') d = c - '0';
        else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
        else return 0;
        if (d >= base) return 0;
        v = v * base + d;
    }

    *out = neg ? -v : v;
    return 1;
}

ufo_token_t *ufo_lex(const char *src, size_t len, int *count) {
    int cap = 1024;
    ufo_token_t *tokens = calloc(cap, sizeof(ufo_token_t));
    if (!tokens) return NULL;
    int n = 0;
    size_t i = 0;
    int line = 1;
    int col = 1;

    #define ENSURE_TOKEN() do { \
        if (n >= cap) { \
            cap *= 2; \
            ufo_token_t *tmp = realloc(tokens, cap * sizeof(ufo_token_t)); \
            if (!tmp) { free(tokens); return NULL; } \
            tokens = tmp; \
            memset(tokens + n, 0, (cap - n) * sizeof(ufo_token_t)); \
        } \
    } while (0)

    while (i < len) {
        while (i < len && (src[i] == ' ' || src[i] == '\t' ||
                           src[i] == '\n' || src[i] == '\r')) {
            if (src[i] == '\n') { line++; col = 1; }
            else col++;
            i++;
        }
        if (i >= len) break;

        int tok_line = line;
        int tok_col = col;

        if (src[i] == '\\') {
            while (i < len && src[i] != '\n') { i++; col++; }
            continue;
        }

        if (src[i] == '(') {
            i++; col++;
            while (i < len && src[i] != ')') {
                if (src[i] == '\n') { line++; col = 1; }
                else col++;
                i++;
            }
            if (i < len) { i++; col++; }
            continue;
        }

        if (src[i] == '.' && i + 1 < len && src[i+1] == '"') {
            ENSURE_TOKEN();
            tokens[n].type = TOK_DOT_QUOTE;
            tokens[n].text = strdup(".\"");
            tokens[n].len = 2;
            tokens[n].line = tok_line;
            tokens[n].col = tok_col;
            n++;
            i += 2;
            col += 2;

            int start = (int)i;
            while (i < len && src[i] != '"') {
                if (src[i] == '\n') { line++; col = 1; }
                else col++;
                i++;
            }
            int slen = (int)i - start;
            ENSURE_TOKEN();
            tokens[n].type = TOK_STRING;
            tokens[n].text = malloc(slen + 1);
            memcpy(tokens[n].text, src + start, slen);
            tokens[n].text[slen] = '\0';
            tokens[n].len = slen;
            tokens[n].line = tok_line;
            tokens[n].col = tok_col;
            n++;
            if (i < len) { i++; col++; }
            continue;
        }

        if (src[i] == 'S' && i + 1 < len && src[i+1] == '"') {
            ENSURE_TOKEN();
            tokens[n].type = TOK_S_QUOTE;
            tokens[n].text = strdup("S\"");
            tokens[n].len = 2;
            tokens[n].line = tok_line;
            tokens[n].col = tok_col;
            n++;
            i += 2;
            col += 2;

            int start = (int)i;
            while (i < len && src[i] != '"') {
                if (src[i] == '\n') { line++; col = 1; }
                else col++;
                i++;
            }
            int slen = (int)i - start;
            ENSURE_TOKEN();
            tokens[n].type = TOK_STRING;
            tokens[n].text = malloc(slen + 1);
            memcpy(tokens[n].text, src + start, slen);
            tokens[n].text[slen] = '\0';
            tokens[n].len = slen;
            tokens[n].line = tok_line;
            tokens[n].col = tok_col;
            n++;
            if (i < len) { i++; col++; }
            continue;
        }

        int start = (int)i;
        while (i < len && src[i] != ' ' && src[i] != '\t' &&
               src[i] != '\n' && src[i] != '\r') {
            i++;
            col++;
        }
        int wlen = (int)i - start;
        if (wlen == 0) continue;

        char *word = malloc(wlen + 1);
        memcpy(word, src + start, wlen);
        word[wlen] = '\0';

        cell_t num;
        if (is_number(word, wlen, &num)) {
            ENSURE_TOKEN();
            tokens[n].type = TOK_NUMBER;
            tokens[n].text = word;
            tokens[n].len = wlen;
            tokens[n].num = num;
            tokens[n].line = tok_line;
            tokens[n].col = tok_col;
            n++;
            continue;
        }

        ufo_tok_type_t t = lookup_keyword(word, wlen);
        ENSURE_TOKEN();
        tokens[n].type = t;
        tokens[n].text = word;
        tokens[n].len = wlen;
        tokens[n].line = tok_line;
        tokens[n].col = tok_col;
        n++;
    }

    ENSURE_TOKEN();
    tokens[n].type = TOK_EOF;
    tokens[n].text = NULL;
    tokens[n].len = 0;
    tokens[n].line = line;
    tokens[n].col = col;
    n++;

    #undef ENSURE_TOKEN

    *count = n;
    return tokens;
}

void ufo_lex_free(ufo_token_t *tokens, int count) {
    for (int i = 0; i < count; i++) {
        if (tokens[i].text) free(tokens[i].text);
    }
    free(tokens);
}