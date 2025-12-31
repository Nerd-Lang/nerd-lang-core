/*
 * NERD Lexer - Tokenizes English-word syntax
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "nerd.h"

/*
 * Keyword table - maps English words to token types
 */
typedef struct {
    const char *word;
    TokenType type;
} Keyword;

static const Keyword keywords[] = {
    // Keywords
    {"fn", TOK_FN},
    {"ret", TOK_RET},
    {"type", TOK_TYPE},
    {"if", TOK_IF},
    {"or", TOK_OR},
    {"ok", TOK_OK},
    {"err", TOK_ERR},
    {"let", TOK_LET},
    {"call", TOK_CALL},

    // Types
    {"num", TOK_NUM},
    {"int", TOK_INT},
    {"str", TOK_STR},
    {"bool", TOK_BOOL},
    {"void", TOK_VOID},

    // Operators
    {"plus", TOK_PLUS},
    {"minus", TOK_MINUS},
    {"times", TOK_TIMES},
    {"over", TOK_OVER},
    {"mod", TOK_MOD},
    {"eq", TOK_EQ},
    {"neq", TOK_NEQ},
    {"lt", TOK_LT},
    {"gt", TOK_GT},
    {"lte", TOK_LTE},
    {"gte", TOK_GTE},
    {"and", TOK_AND},
    {"not", TOK_NOT},

    // Positional
    {"first", TOK_FIRST},
    {"second", TOK_SECOND},
    {"third", TOK_THIRD},
    {"fourth", TOK_FOURTH},

    // Number words
    {"zero", TOK_ZERO},
    {"one", TOK_ONE},
    {"two", TOK_TWO},
    {"three", TOK_THREE},
    {"four", TOK_FOUR},
    {"five", TOK_FIVE},
    {"six", TOK_SIX},
    {"seven", TOK_SEVEN},
    {"eight", TOK_EIGHT},
    {"nine", TOK_NINE},
    {"ten", TOK_TEN},

    // Modules
    {"math", TOK_MATH},
    {"list", TOK_LIST},
    {"time", TOK_TIME},
    {"http", TOK_HTTP},
    {"json", TOK_JSON},

    {NULL, TOK_EOF}
};

/*
 * Create a new lexer
 */
Lexer *lexer_create(const char *source, size_t len) {
    Lexer *lexer = malloc(sizeof(Lexer));
    if (!lexer) return NULL;

    lexer->source = source;
    lexer->source_len = len;
    lexer->pos = 0;
    lexer->line = 1;
    lexer->column = 1;

    lexer->token_capacity = 256;
    lexer->token_count = 0;
    lexer->tokens = malloc(sizeof(Token) * lexer->token_capacity);
    if (!lexer->tokens) {
        free(lexer);
        return NULL;
    }

    return lexer;
}

/*
 * Free lexer and all tokens
 */
void lexer_free(Lexer *lexer) {
    if (!lexer) return;

    for (size_t i = 0; i < lexer->token_count; i++) {
        free(lexer->tokens[i].value);
    }
    free(lexer->tokens);
    free(lexer);
}

/*
 * Add a token to the list
 */
static bool lexer_add_token(Lexer *lexer, TokenType type, const char *value, size_t len) {
    if (lexer->token_count >= lexer->token_capacity) {
        lexer->token_capacity *= 2;
        Token *new_tokens = realloc(lexer->tokens, sizeof(Token) * lexer->token_capacity);
        if (!new_tokens) return false;
        lexer->tokens = new_tokens;
    }

    Token *tok = &lexer->tokens[lexer->token_count++];
    tok->type = type;
    tok->value = nerd_strndup(value, len);
    tok->value_len = len;
    tok->line = lexer->line;
    tok->column = lexer->column - (int)len;

    return true;
}

/*
 * Current character
 */
static char lexer_current(Lexer *lexer) {
    if (lexer->pos >= lexer->source_len) return '\0';
    return lexer->source[lexer->pos];
}

/*
 * Peek next character
 */
static char lexer_peek(Lexer *lexer) {
    if (lexer->pos + 1 >= lexer->source_len) return '\0';
    return lexer->source[lexer->pos + 1];
}

/*
 * Advance position
 */
static void lexer_advance(Lexer *lexer) {
    if (lexer->pos < lexer->source_len) {
        lexer->pos++;
        lexer->column++;
    }
}

/*
 * Check if character is alphanumeric or underscore
 */
static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_alnum(char c) {
    return is_alpha(c) || is_digit(c);
}

/*
 * Look up a word in the keyword table
 */
static TokenType lookup_keyword(const char *word, size_t len) {
    for (int i = 0; keywords[i].word != NULL; i++) {
        if (strlen(keywords[i].word) == len &&
            strncmp(keywords[i].word, word, len) == 0) {
            return keywords[i].type;
        }
    }
    return TOK_IDENT;
}

/*
 * Scan a word (keyword or identifier)
 */
static bool lexer_scan_word(Lexer *lexer) {
    size_t start = lexer->pos;

    while (is_alnum(lexer_current(lexer))) {
        lexer_advance(lexer);
    }

    size_t len = lexer->pos - start;
    const char *word = lexer->source + start;

    TokenType type = lookup_keyword(word, len);
    return lexer_add_token(lexer, type, word, len);
}

/*
 * Scan a number
 */
static bool lexer_scan_number(Lexer *lexer) {
    size_t start = lexer->pos;

    while (is_digit(lexer_current(lexer))) {
        lexer_advance(lexer);
    }

    // Decimal part
    if (lexer_current(lexer) == '.' && is_digit(lexer_peek(lexer))) {
        lexer_advance(lexer);  // Skip '.'
        while (is_digit(lexer_current(lexer))) {
            lexer_advance(lexer);
        }
    }

    size_t len = lexer->pos - start;
    return lexer_add_token(lexer, TOK_NUMBER, lexer->source + start, len);
}

/*
 * Scan a string literal
 */
static bool lexer_scan_string(Lexer *lexer) {
    lexer_advance(lexer);  // Skip opening "
    size_t start = lexer->pos;

    while (lexer_current(lexer) != '"' && lexer_current(lexer) != '\0') {
        if (lexer_current(lexer) == '\n') {
            fprintf(stderr, "Error: Unterminated string at line %d\n", lexer->line);
            return false;
        }
        if (lexer_current(lexer) == '\\' && lexer_peek(lexer) == '"') {
            lexer_advance(lexer);  // Skip escape
        }
        lexer_advance(lexer);
    }

    if (lexer_current(lexer) == '\0') {
        fprintf(stderr, "Error: Unterminated string at line %d\n", lexer->line);
        return false;
    }

    size_t len = lexer->pos - start;
    bool ok = lexer_add_token(lexer, TOK_STRING, lexer->source + start, len);
    lexer_advance(lexer);  // Skip closing "
    return ok;
}

/*
 * Tokenize the source
 */
bool lexer_tokenize(Lexer *lexer) {
    while (lexer->pos < lexer->source_len) {
        char c = lexer_current(lexer);

        // Skip whitespace (except newlines)
        if (c == ' ' || c == '\t' || c == '\r') {
            lexer_advance(lexer);
            continue;
        }

        // Newlines
        if (c == '\n') {
            lexer_add_token(lexer, TOK_NEWLINE, "\\n", 2);
            lexer_advance(lexer);
            lexer->line++;
            lexer->column = 1;
            continue;
        }

        // Comments: -- or #
        if ((c == '-' && lexer_peek(lexer) == '-') || c == '#') {
            while (lexer_current(lexer) != '\n' && lexer_current(lexer) != '\0') {
                lexer_advance(lexer);
            }
            continue;
        }

        // String literals
        if (c == '"') {
            if (!lexer_scan_string(lexer)) return false;
            continue;
        }

        // Numbers
        if (is_digit(c)) {
            if (!lexer_scan_number(lexer)) return false;
            continue;
        }

        // Words (keywords and identifiers)
        if (is_alpha(c)) {
            if (!lexer_scan_word(lexer)) return false;
            continue;
        }

        // Unknown character
        fprintf(stderr, "Error: Unexpected character '%c' at line %d, column %d\n",
                c, lexer->line, lexer->column);
        return false;
    }

    // Add EOF token
    lexer_add_token(lexer, TOK_EOF, "", 0);
    return true;
}

/*
 * String duplication utilities
 */
char *nerd_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *dup = malloc(len + 1);
    if (dup) {
        memcpy(dup, s, len + 1);
    }
    return dup;
}

char *nerd_strndup(const char *s, size_t n) {
    if (!s) return NULL;
    char *dup = malloc(n + 1);
    if (dup) {
        memcpy(dup, s, n);
        dup[n] = '\0';
    }
    return dup;
}
