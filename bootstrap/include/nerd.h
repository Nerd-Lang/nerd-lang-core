/*
 * NERD Bootstrap Compiler
 * No Effort Required, Done
 *
 * A language optimized for LLM generation, not human authorship.
 * Plain English words. Dense. Machine-friendly.
 */

#ifndef NERD_H
#define NERD_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*
 * Token Types - All English words, each tokenizes as 1 LLM token
 */
typedef enum {
    // Keywords
    TOK_FN,         // fn - function definition
    TOK_RET,        // ret - return
    TOK_TYPE,       // type - type definition
    TOK_IF,         // if - conditional
    TOK_ELSE,       // else - else branch
    TOK_OR,         // or - union separator / logical or
    TOK_OK,         // ok - success variant
    TOK_ERR,        // err - error variant
    TOK_LET,        // let - variable binding
    TOK_CALL,       // call - function call
    TOK_OUT,        // out - output/print
    TOK_DONE,       // done - block terminator
    TOK_REPEAT,     // repeat - loop start
    TOK_AS,         // as - loop variable binding
    TOK_WHILE,      // while - conditional loop
    TOK_NEG,        // neg - negation
    TOK_INC,        // inc - increment
    TOK_DEC,        // dec - decrement
    TOK_COUNT,      // count - array length

    // JSON/Object operators
    TOK_LBRACE,     // {
    TOK_RBRACE,     // }
    TOK_DOT,        // .
    TOK_QUESTION,   // ?
    TOK_ASSIGN,     // =

    // Types
    TOK_NUM,        // num - f64
    TOK_INT,        // int - i64
    TOK_STR,        // str - string
    TOK_BOOL,       // bool - boolean
    TOK_VOID,       // void - no value

    // Operators
    TOK_PLUS,       // plus
    TOK_MINUS,      // minus
    TOK_TIMES,      // times
    TOK_OVER,       // over (division)
    TOK_MOD,        // mod
    TOK_EQ,         // eq
    TOK_NEQ,        // neq
    TOK_LT,         // lt
    TOK_GT,         // gt
    TOK_LTE,        // lte
    TOK_GTE,        // gte
    TOK_AND,        // and
    TOK_NOT,        // not

    // Positional references
    TOK_FIRST,      // first - param 0
    TOK_SECOND,     // second - param 1
    TOK_THIRD,      // third - param 2
    TOK_FOURTH,     // fourth - param 3

    // Number words
    TOK_ZERO,
    TOK_ONE,
    TOK_TWO,
    TOK_THREE,
    TOK_FOUR,
    TOK_FIVE,
    TOK_SIX,
    TOK_SEVEN,
    TOK_EIGHT,
    TOK_NINE,
    TOK_TEN,

    // Standard library modules
    TOK_MATH,       // math module
    TOK_LIST,       // list module
    TOK_TIME,       // time module
    TOK_HTTP,       // http module
    TOK_JSON,       // json module
    TOK_MCP,        // mcp module (Model Context Protocol)
    TOK_LLM,        // llm module (Claude, OpenAI, etc.)

    // HTTP methods
    TOK_GET,        // get
    TOK_POST,       // post
    TOK_PUT,        // put
    TOK_DELETE,     // delete
    TOK_PATCH,      // patch

    // HTTP modifiers
    TOK_WITH,       // with - for headers
    TOK_AUTH,       // auth - authentication
    TOK_BEARER,     // bearer - bearer token
    TOK_BASIC,      // basic - basic auth

    // Literals and identifiers
    TOK_NUMBER,     // numeric literal
    TOK_STRING,     // string literal
    TOK_IDENT,      // identifier

    // Special
    TOK_NEWLINE,
    TOK_EOF,
} TokenType;

/*
 * Token structure
 */
typedef struct {
    TokenType type;
    char *value;        // Token text
    size_t value_len;
    int line;
    int column;
} Token;

/*
 * Lexer state
 */
typedef struct {
    const char *source;
    size_t source_len;
    size_t pos;
    int line;
    int column;

    Token *tokens;
    size_t token_count;
    size_t token_capacity;
} Lexer;

/*
 * AST Node Types
 */
typedef enum {
    NODE_PROGRAM,
    NODE_FUNC_DEF,
    NODE_TYPE_DEF,
    NODE_PARAM,
    NODE_RETURN,
    NODE_IF,
    NODE_LET,
    NODE_EXPR_STMT,
    NODE_OUT,
    NODE_REPEAT,
    NODE_WHILE,
    NODE_INC,
    NODE_DEC,
    NODE_BINOP,
    NODE_UNARYOP,
    NODE_CALL,
    NODE_NUM,
    NODE_STR,
    NODE_BOOL,
    NODE_VAR,
    NODE_POSITIONAL,
    NODE_JSON_NEW,      // let x {}
    NODE_JSON_ACCESS,   // x."path"
    NODE_JSON_HAS,      // x?"key"
    NODE_JSON_COUNT,    // x."path".count
    NODE_JSON_SET,      // x."key" = value
} NodeType;

/*
 * Forward declarations
 */
typedef struct ASTNode ASTNode;
typedef struct ASTList ASTList;

/*
 * List of AST nodes (for function bodies, parameters, etc.)
 */
struct ASTList {
    ASTNode **nodes;
    size_t count;
    size_t capacity;
};

/*
 * AST Node
 */
struct ASTNode {
    NodeType type;
    int line;

    union {
        // Program
        struct {
            ASTList types;
            ASTList functions;
        } program;

        // Function definition
        struct {
            char *name;
            ASTList params;
            ASTNode *return_type;
            ASTList body;
        } func_def;

        // Type definition
        struct {
            char *name;
            bool is_union;      // struct vs union (ok/err)
            ASTList fields;     // struct fields
            ASTNode *ok_type;   // for union
            ASTNode *err_type;  // for union
        } type_def;

        // Parameter
        struct {
            char *name;
            ASTNode *param_type;
        } param;

        // Return statement
        struct {
            int variant;        // 0=none, 1=ok, 2=err
            ASTNode *value;
        } ret;

        // If statement
        struct {
            ASTNode *condition;
            ASTNode *then_stmt;
            ASTNode *else_stmt;     // NULL if no else branch
        } if_stmt;

        // Out statement (output/print)
        struct {
            ASTNode *value;
        } out;

        // Repeat loop (repeat n times [as i] ... done)
        struct {
            ASTNode *count;         // expression for iteration count
            char *var_name;         // optional "as i" variable (NULL if not present)
            ASTList body;           // loop body
        } repeat;

        // While loop (while cond ... done)
        struct {
            ASTNode *condition;     // loop condition
            ASTList body;           // loop body
        } while_loop;

        // Increment statement (inc var [amount])
        struct {
            char *var_name;
            ASTNode *amount;        // NULL means increment by 1
        } inc;

        // Decrement statement (dec var [amount])
        struct {
            char *var_name;
            ASTNode *amount;        // NULL means decrement by 1
        } dec;

        // Let binding
        struct {
            char *name;
            ASTNode *value;
        } let;

        // Expression statement
        struct {
            ASTNode *expr;
        } expr_stmt;

        // Binary operation
        struct {
            char *op;           // plus, minus, times, over, eq, etc.
            ASTNode *left;
            ASTNode *right;
        } binop;

        // Unary operation
        struct {
            char *op;           // not
            ASTNode *operand;
        } unaryop;

        // Function/module call
        struct {
            char *module;       // NULL if not a module call
            char *func;
            ASTList args;
        } call;

        // Number literal
        struct {
            double value;
        } num;

        // String literal
        struct {
            char *value;
        } str;

        // Boolean literal
        struct {
            bool value;
        } boolean;

        // Variable reference
        struct {
            char *name;
        } var;

        // Positional parameter reference
        struct {
            int index;          // 0=first, 1=second, etc.
        } positional;

        // JSON access (x."path")
        struct {
            ASTNode *object;    // The JSON object being accessed
            char *path;         // The path string (e.g., "user.name" or "items[0]")
        } json_access;

        // JSON has check (x?"key")
        struct {
            ASTNode *object;
            char *path;
        } json_has;

        // JSON count (x."path".count)
        struct {
            ASTNode *object;
            char *path;         // Path to array (can be NULL for root)
        } json_count;

        // JSON set (x."key" = value)
        struct {
            ASTNode *object;
            char *key;
            ASTNode *value;
        } json_set;
    } data;
};

/*
 * Parser state
 */
typedef struct {
    Token *tokens;
    size_t token_count;
    size_t pos;
} Parser;

/*
 * Compiler context
 */
typedef struct {
    const char *filename;
    const char *source;
    ASTNode *ast;

    // Error handling
    char *error_msg;
    int error_line;
} NerdContext;

/*
 * Lexer functions
 */
Lexer *lexer_create(const char *source, size_t len);
void lexer_free(Lexer *lexer);
bool lexer_tokenize(Lexer *lexer);

/*
 * Parser functions
 */
Parser *parser_create(Token *tokens, size_t count);
void parser_free(Parser *parser);
ASTNode *parser_parse(Parser *parser);

/*
 * AST functions
 */
ASTNode *ast_create(NodeType type, int line);
void ast_free(ASTNode *node);
void ast_list_init(ASTList *list);
void ast_list_push(ASTList *list, ASTNode *node);
void ast_list_free(ASTList *list);

/*
 * Code generation (LLVM)
 */
bool codegen_llvm(NerdContext *ctx, const char *output_path);

/*
 * Utility functions
 */
char *nerd_strdup(const char *s);
char *nerd_strndup(const char *s, size_t n);

#endif /* NERD_H */
