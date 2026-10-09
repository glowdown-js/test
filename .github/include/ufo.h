#ifndef UFO_H
#define UFO_H

#include <stddef.h>
#include <stdint.h>

#define UFO_VERSION_STRING "0.1.2"
#define UFO_STACK_SIZE 4096
#define UFO_DICT_SIZE  8192
#define UFO_WORD_MAX   64
#define UFO_IR_MAX     65536
#define UFO_CODE_MAX   65536
#define UFO_STR_MAX    256
#define UFO_VAR_MAX    1024
#define UFO_CONST_MAX  1024

typedef int64_t cell_t;

typedef enum {
    TOK_EOF = 0, TOK_WORD, TOK_NUMBER, TOK_STRING,
    TOK_COLON, TOK_SEMICOLON, TOK_IF, TOK_ELSE, TOK_THEN,
    TOK_DO, TOK_LOOP, TOK_PLUSLOOP, TOK_QDO,
    TOK_BEGIN, TOK_UNTIL, TOK_WHILE, TOK_REPEAT, TOK_AGAIN,
    TOK_LEAVE, TOK_UNLOOP, TOK_I, TOK_J, TOK_K,
    TOK_CASE, TOK_OF, TOK_ENDOF, TOK_ENDCASE,
    TOK_DOT_QUOTE, TOK_S_QUOTE,
    TOK_VARIABLE, TOK_CONSTANT, TOK_VALUE, TOK_TO,
    TOK_CREATE, TOK_ALLOT, TOK_HERE,
    TOK_2FETCH, TOK_2STORE,
    TOK_CELLPLUS, TOK_CELLS, TOK_CHARPLUS, TOK_CHARS,
    TOK_ALIGN, TOK_ALIGNED, TOK_PAD,
    TOK_COUNT, TOK_CMOVE, TOK_CMOVE_UP, TOK_MOVE, TOK_FILL,
    TOK_ERASE, TOK_BLANK, TOK_COMPARE, TOK_SEARCH,
    TOK_MINUS_TRAILING, TOK_SLASH_STRING,
    TOK_BL, TOK_CHAR, TOK_BRACKET_CHAR,
    TOK_IMMEDIATE, TOK_RECURSE, TOK_EXIT,
    TOK_DUP, TOK_DROP, TOK_SWAP, TOK_OVER, TOK_ROT,
    TOK_2DUP, TOK_2DROP, TOK_2SWAP, TOK_2OVER,
    TOK_NIP, TOK_TUCK, TOK_QDUP, TOK_DEPTH, TOK_PICK,
    TOK_TO_R, TOK_R_FROM, TOK_R_FETCH,
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_MOD,
    TOK_SLASH_MOD, TOK_STAR_SLASH, TOK_STAR_SLASH_MOD,
    TOK_ABS, TOK_NEGATE, TOK_MIN, TOK_MAXWORD,
    TOK_1PLUS, TOK_1MINUS, TOK_2PLUS, TOK_2MINUS, TOK_2STAR, TOK_2SLASH,
    TOK_EQ, TOK_NE, TOK_LT, TOK_GT, TOK_LE, TOK_GE,
    TOK_ZERO_EQ, TOK_ZERO_LT, TOK_ZERO_GT,
    TOK_AND, TOK_OR, TOK_XOR, TOK_INVERT, TOK_LSHIFT, TOK_RSHIFT,
    TOK_EMIT, TOK_CR, TOK_DOT, TOK_DOTS, TOK_TYPE,
    TOK_KEY, TOK_KEY_Q, TOK_ACCEPT,
    TOK_U_DOT, TOK_H_DOT, TOK_SPACE, TOK_SPACES,
    TOK_LOAD, TOK_STORE, TOK_CFETCH, TOK_CSTORE, TOK_ADD_STORE, TOK_FETCH,
    TOK_COMMENT, TOK_COMMENT_PAREN,
    TOK_PARSE, TOK_WORD_W, TOK_TO_NUMBER, TOK_RANDOM, TOK_INPUT,
    TOK_END
} ufo_tok_type_t;

typedef struct {
    ufo_tok_type_t type;
    char          *text;
    size_t         len;
    cell_t         num;
    int            line;
    int            col;
} ufo_token_t;

ufo_token_t *ufo_lex(const char *src, size_t len, int *count);
void         ufo_lex_free(ufo_token_t *tokens, int count);

int          ufo_compile_to_c(ufo_token_t *tokens, int count, const char *filename);
const char  *ufo_version(void);

#endif