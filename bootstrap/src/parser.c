/*
 * NERD Parser - Builds AST from tokens
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nerd.h"

/*
 * AST Node creation
 */
ASTNode *ast_create(NodeType type, int line) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    if (node) {
        node->type = type;
        node->line = line;
    }
    return node;
}

/*
 * Free AST node recursively
 */
void ast_free(ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_PROGRAM:
            ast_list_free(&node->data.program.types);
            ast_list_free(&node->data.program.functions);
            break;
        case NODE_FUNC_DEF:
            free(node->data.func_def.name);
            ast_list_free(&node->data.func_def.params);
            ast_free(node->data.func_def.return_type);
            ast_list_free(&node->data.func_def.body);
            break;
        case NODE_TYPE_DEF:
            free(node->data.type_def.name);
            ast_list_free(&node->data.type_def.fields);
            ast_free(node->data.type_def.ok_type);
            ast_free(node->data.type_def.err_type);
            break;
        case NODE_PARAM:
            free(node->data.param.name);
            ast_free(node->data.param.param_type);
            break;
        case NODE_RETURN:
            ast_free(node->data.ret.value);
            break;
        case NODE_IF:
            ast_free(node->data.if_stmt.condition);
            ast_free(node->data.if_stmt.then_stmt);
            ast_free(node->data.if_stmt.else_stmt);
            break;
        case NODE_OUT:
            ast_free(node->data.out.value);
            break;
        case NODE_REPEAT:
            ast_free(node->data.repeat.count);
            free(node->data.repeat.var_name);
            ast_list_free(&node->data.repeat.body);
            break;
        case NODE_WHILE:
            ast_free(node->data.while_loop.condition);
            ast_list_free(&node->data.while_loop.body);
            break;
        case NODE_INC:
            free(node->data.inc.var_name);
            ast_free(node->data.inc.amount);
            break;
        case NODE_DEC:
            free(node->data.dec.var_name);
            ast_free(node->data.dec.amount);
            break;
        case NODE_LET:
            free(node->data.let.name);
            ast_free(node->data.let.value);
            break;
        case NODE_EXPR_STMT:
            ast_free(node->data.expr_stmt.expr);
            break;
        case NODE_BINOP:
            free(node->data.binop.op);
            ast_free(node->data.binop.left);
            ast_free(node->data.binop.right);
            break;
        case NODE_UNARYOP:
            free(node->data.unaryop.op);
            ast_free(node->data.unaryop.operand);
            break;
        case NODE_CALL:
            free(node->data.call.module);
            free(node->data.call.func);
            ast_list_free(&node->data.call.args);
            break;
        case NODE_STR:
            free(node->data.str.value);
            break;
        case NODE_VAR:
            free(node->data.var.name);
            break;
        case NODE_JSON_NEW:
            // No additional data to free
            break;
        case NODE_JSON_ACCESS:
            ast_free(node->data.json_access.object);
            free(node->data.json_access.path);
            break;
        case NODE_JSON_HAS:
            ast_free(node->data.json_has.object);
            free(node->data.json_has.path);
            break;
        case NODE_JSON_COUNT:
            ast_free(node->data.json_count.object);
            free(node->data.json_count.path);
            break;
        case NODE_JSON_SET:
            ast_free(node->data.json_set.object);
            free(node->data.json_set.key);
            ast_free(node->data.json_set.value);
            break;
        default:
            break;
    }

    free(node);
}

/*
 * AST List operations
 */
void ast_list_init(ASTList *list) {
    list->nodes = NULL;
    list->count = 0;
    list->capacity = 0;
}

void ast_list_push(ASTList *list, ASTNode *node) {
    if (list->count >= list->capacity) {
        size_t new_capacity = list->capacity == 0 ? 8 : list->capacity * 2;
        ASTNode **new_nodes = realloc(list->nodes, sizeof(ASTNode*) * new_capacity);
        if (!new_nodes) {
            fprintf(stderr, "Error: Out of memory\n");
            return;
        }
        list->nodes = new_nodes;
        list->capacity = new_capacity;
    }
    list->nodes[list->count++] = node;
}

void ast_list_free(ASTList *list) {
    for (size_t i = 0; i < list->count; i++) {
        ast_free(list->nodes[i]);
    }
    free(list->nodes);
    list->nodes = NULL;
    list->count = 0;
    list->capacity = 0;
}

/*
 * Parser creation
 */
Parser *parser_create(Token *tokens, size_t count) {
    Parser *parser = malloc(sizeof(Parser));
    if (!parser) return NULL;

    parser->tokens = tokens;
    parser->token_count = count;
    parser->pos = 0;

    return parser;
}

void parser_free(Parser *parser) {
    free(parser);
}

/*
 * Parser helpers
 */
static Token *parser_current(Parser *parser) {
    return &parser->tokens[parser->pos];
}

static bool parser_check(Parser *parser, TokenType type) {
    return parser_current(parser)->type == type;
}

static bool parser_at_end(Parser *parser) {
    return parser_current(parser)->type == TOK_EOF;
}

static Token *parser_advance(Parser *parser) {
    if (!parser_at_end(parser)) parser->pos++;
    return &parser->tokens[parser->pos - 1];
}

static bool parser_match(Parser *parser, TokenType type) {
    if (parser_check(parser, type)) {
        parser_advance(parser);
        return true;
    }
    return false;
}

static Token *parser_expect(Parser *parser, TokenType type, const char *msg) {
    if (parser_check(parser, type)) {
        return parser_advance(parser);
    }
    fprintf(stderr, "Error at line %d: %s (got %d)\n",
            parser_current(parser)->line, msg, parser_current(parser)->type);
    return NULL;
}

static void parser_skip_newlines(Parser *parser) {
    while (parser_match(parser, TOK_NEWLINE));
}

static bool parser_at_end_of_line(Parser *parser) {
    return parser_check(parser, TOK_NEWLINE) || parser_check(parser, TOK_EOF);
}

/*
 * Check if current token is a type token
 */
static bool is_type_token(Parser *parser) {
    TokenType t = parser_current(parser)->type;
    return t == TOK_NUM || t == TOK_INT || t == TOK_STR ||
           t == TOK_BOOL || t == TOK_VOID || t == TOK_LIST;
}

/*
 * Check if current token is a module token
 */
static bool is_module_token(Parser *parser) {
    TokenType t = parser_current(parser)->type;
    return t == TOK_MATH || t == TOK_STR || t == TOK_LIST ||
           t == TOK_TIME || t == TOK_HTTP || t == TOK_JSON || t == TOK_ERR ||
           t == TOK_MCP || t == TOK_LLM;
}

/*
 * Check if current token ends an expression
 */
static bool is_end_of_expr(Parser *parser) {
    TokenType t = parser_current(parser)->type;
    return parser_at_end_of_line(parser) ||
           t == TOK_PLUS || t == TOK_MINUS || t == TOK_TIMES ||
           t == TOK_OVER || t == TOK_MOD || t == TOK_EQ ||
           t == TOK_NEQ || t == TOK_LT || t == TOK_GT ||
           t == TOK_LTE || t == TOK_GTE || t == TOK_AND ||
           t == TOK_OR || t == TOK_RET || t == TOK_LET ||
           t == TOK_IF || t == TOK_ELSE || t == TOK_CALL ||
           t == TOK_OUT || t == TOK_DONE || t == TOK_REPEAT ||
           t == TOK_TIMES || t == TOK_AS || t == TOK_WHILE ||
           t == TOK_ASSIGN || t == TOK_RBRACE;
}

/*
 * Check if token is a number word
 */
static bool is_number_word(Parser *parser) {
    TokenType t = parser_current(parser)->type;
    return t >= TOK_ZERO && t <= TOK_TEN;
}

/*
 * Get value of number word
 */
static double number_word_value(TokenType type) {
    switch (type) {
        case TOK_ZERO: return 0;
        case TOK_ONE: return 1;
        case TOK_TWO: return 2;
        case TOK_THREE: return 3;
        case TOK_FOUR: return 4;
        case TOK_FIVE: return 5;
        case TOK_SIX: return 6;
        case TOK_SEVEN: return 7;
        case TOK_EIGHT: return 8;
        case TOK_NINE: return 9;
        case TOK_TEN: return 10;
        default: return 0;
    }
}

/*
 * Check if token is a positional reference
 */
static bool is_positional(Parser *parser) {
    TokenType t = parser_current(parser)->type;
    return t == TOK_FIRST || t == TOK_SECOND ||
           t == TOK_THIRD || t == TOK_FOURTH;
}

/*
 * Get positional index
 */
static int positional_index(TokenType type) {
    switch (type) {
        case TOK_FIRST: return 0;
        case TOK_SECOND: return 1;
        case TOK_THIRD: return 2;
        case TOK_FOURTH: return 3;
        default: return 0;
    }
}

/*
 * Forward declarations for recursive descent
 */
static ASTNode *parse_expr(Parser *parser);
static ASTNode *parse_stmt(Parser *parser);
static ASTNode *parse_inline_stmt(Parser *parser);
static ASTNode *parse_unary(Parser *parser);
static ASTNode *parse_postfix(Parser *parser);

/*
 * Parse primary expression
 */
static ASTNode *parse_primary(Parser *parser) {
    int line = parser_current(parser)->line;

    // Number literal
    if (parser_check(parser, TOK_NUMBER)) {
        Token *tok = parser_advance(parser);
        ASTNode *node = ast_create(NODE_NUM, line);
        node->data.num.value = atof(tok->value);
        return node;
    }

    // String literal
    if (parser_check(parser, TOK_STRING)) {
        Token *tok = parser_advance(parser);
        ASTNode *node = ast_create(NODE_STR, line);
        node->data.str.value = nerd_strdup(tok->value);
        return node;
    }

    // Number words
    if (is_number_word(parser)) {
        Token *tok = parser_advance(parser);
        ASTNode *node = ast_create(NODE_NUM, line);
        node->data.num.value = number_word_value(tok->type);
        return node;
    }

    // Positional references
    if (is_positional(parser)) {
        Token *tok = parser_advance(parser);
        ASTNode *node = ast_create(NODE_POSITIONAL, line);
        node->data.positional.index = positional_index(tok->type);
        return node;
    }

    // Boolean literals
    if (parser_check(parser, TOK_IDENT)) {
        if (strcmp(parser_current(parser)->value, "true") == 0) {
            parser_advance(parser);
            ASTNode *node = ast_create(NODE_BOOL, line);
            node->data.boolean.value = true;
            return node;
        }
        if (strcmp(parser_current(parser)->value, "false") == 0) {
            parser_advance(parser);
            ASTNode *node = ast_create(NODE_BOOL, line);
            node->data.boolean.value = false;
            return node;
        }
    }

    // Variable
    if (parser_check(parser, TOK_IDENT)) {
        Token *tok = parser_advance(parser);
        ASTNode *node = ast_create(NODE_VAR, line);
        node->data.var.name = nerd_strdup(tok->value);
        return node;
    }

    fprintf(stderr, "Error at line %d: Unexpected token in expression\n", line);
    return NULL;
}

/*
 * Parse postfix operations: obj."path", obj?"key", obj."path".count
 */
static ASTNode *parse_postfix(Parser *parser) {
    ASTNode *left = parse_primary(parser);
    if (!left) return NULL;

    int line = parser_current(parser)->line;

    // Handle chained postfix operations
    while (parser_check(parser, TOK_DOT) || parser_check(parser, TOK_QUESTION)) {
        // JSON access: obj."path"
        if (parser_match(parser, TOK_DOT)) {
            // Expect a string literal for the path
            if (!parser_check(parser, TOK_STRING)) {
                // Check for .count suffix
                if (parser_match(parser, TOK_COUNT)) {
                    // obj.count (count of root array) or after json_access.count
                    ASTNode *count_node = ast_create(NODE_JSON_COUNT, line);
                    count_node->data.json_count.object = left;
                    count_node->data.json_count.path = NULL;  // Root level count
                    left = count_node;
                    continue;
                }
                fprintf(stderr, "Error at line %d: Expected string path after '.'\n", line);
                ast_free(left);
                return NULL;
            }

            Token *path_tok = parser_advance(parser);
            
            // Check if followed by .count
            if (parser_check(parser, TOK_DOT)) {
                // Peek ahead to see if it's .count
                size_t saved_pos = parser->pos;
                parser_advance(parser);  // consume .
                if (parser_match(parser, TOK_COUNT)) {
                    // obj."path".count
                    ASTNode *count_node = ast_create(NODE_JSON_COUNT, line);
                    count_node->data.json_count.object = left;
                    count_node->data.json_count.path = nerd_strdup(path_tok->value);
                    left = count_node;
                    continue;
                } else {
                    // Restore position - it's a chained access
                    parser->pos = saved_pos;
                }
            }

            // Regular JSON access
            ASTNode *access_node = ast_create(NODE_JSON_ACCESS, line);
            access_node->data.json_access.object = left;
            access_node->data.json_access.path = nerd_strdup(path_tok->value);
            left = access_node;
        }
        // JSON has: obj?"key"
        else if (parser_match(parser, TOK_QUESTION)) {
            if (!parser_check(parser, TOK_STRING)) {
                fprintf(stderr, "Error at line %d: Expected string key after '?'\n", line);
                ast_free(left);
                return NULL;
            }

            Token *key_tok = parser_advance(parser);
            ASTNode *has_node = ast_create(NODE_JSON_HAS, line);
            has_node->data.json_has.object = left;
            has_node->data.json_has.path = nerd_strdup(key_tok->value);
            left = has_node;
        }
    }

    return left;
}

/*
 * Parse function call (module call or user-defined function call)
 */
static ASTNode *parse_call(Parser *parser) {
    int line = parser_current(parser)->line;

    // User-defined function call: call funcname arg1 arg2 ...
    if (parser_match(parser, TOK_CALL)) {
        Token *func_tok = parser_expect(parser, TOK_IDENT, "Expected function name after call");
        if (!func_tok) return NULL;

        ASTNode *node = ast_create(NODE_CALL, line);
        node->data.call.module = NULL;  // No module = user-defined function
        node->data.call.func = nerd_strdup(func_tok->value);
        ast_list_init(&node->data.call.args);

        // Parse arguments until end of expression context
        while (!is_end_of_expr(parser)) {
            ASTNode *arg = parse_unary(parser);  // Allow unary ops (neg, not)
            if (!arg) {
                ast_free(node);
                return NULL;
            }
            ast_list_push(&node->data.call.args, arg);
        }

        return node;
    }

    // Module call: math abs x, http get url, etc.
    if (is_module_token(parser)) {
        Token *mod_tok = parser_advance(parser);
        Token *func_tok = parser_expect(parser, TOK_IDENT, "Expected function name after module");
        if (!func_tok) return NULL;

        ASTNode *node = ast_create(NODE_CALL, line);
        node->data.call.module = nerd_strdup(mod_tok->value);
        node->data.call.func = nerd_strdup(func_tok->value);
        ast_list_init(&node->data.call.args);

        // Parse arguments until end of expression context
        while (!is_end_of_expr(parser)) {
            ASTNode *arg = parse_unary(parser);  // Allow unary ops (neg, not)
            if (!arg) {
                ast_free(node);
                return NULL;
            }
            ast_list_push(&node->data.call.args, arg);
        }

        return node;
    }

    return parse_postfix(parser);
}

/*
 * Parse unary expression
 */
static ASTNode *parse_unary(Parser *parser) {
    int line = parser_current(parser)->line;

    if (parser_match(parser, TOK_NOT)) {
        ASTNode *operand = parse_unary(parser);
        if (!operand) return NULL;

        ASTNode *node = ast_create(NODE_UNARYOP, line);
        node->data.unaryop.op = nerd_strdup("not");
        node->data.unaryop.operand = operand;
        return node;
    }

    // neg x -> negate x
    if (parser_match(parser, TOK_NEG)) {
        ASTNode *operand = parse_unary(parser);
        if (!operand) return NULL;

        ASTNode *node = ast_create(NODE_UNARYOP, line);
        node->data.unaryop.op = nerd_strdup("neg");
        node->data.unaryop.operand = operand;
        return node;
    }

    return parse_call(parser);
}

/*
 * Parse multiplicative expression
 */
static ASTNode *parse_multiplicative(Parser *parser) {
    ASTNode *left = parse_unary(parser);
    if (!left) return NULL;

    while (parser_check(parser, TOK_TIMES) ||
           parser_check(parser, TOK_OVER) ||
           parser_check(parser, TOK_MOD)) {
        int line = parser_current(parser)->line;
        Token *op_tok = parser_advance(parser);

        ASTNode *right = parse_unary(parser);
        if (!right) {
            ast_free(left);
            return NULL;
        }

        ASTNode *node = ast_create(NODE_BINOP, line);
        node->data.binop.op = nerd_strdup(op_tok->value);
        node->data.binop.left = left;
        node->data.binop.right = right;
        left = node;
    }

    return left;
}

/*
 * Parse additive expression
 */
static ASTNode *parse_additive(Parser *parser) {
    ASTNode *left = parse_multiplicative(parser);
    if (!left) return NULL;

    while (parser_check(parser, TOK_PLUS) || parser_check(parser, TOK_MINUS)) {
        int line = parser_current(parser)->line;
        Token *op_tok = parser_advance(parser);

        ASTNode *right = parse_multiplicative(parser);
        if (!right) {
            ast_free(left);
            return NULL;
        }

        ASTNode *node = ast_create(NODE_BINOP, line);
        node->data.binop.op = nerd_strdup(op_tok->value);
        node->data.binop.left = left;
        node->data.binop.right = right;
        left = node;
    }

    return left;
}

/*
 * Parse comparison expression
 */
static ASTNode *parse_comparison(Parser *parser) {
    ASTNode *left = parse_additive(parser);
    if (!left) return NULL;

    while (parser_check(parser, TOK_EQ) || parser_check(parser, TOK_NEQ) ||
           parser_check(parser, TOK_LT) || parser_check(parser, TOK_GT) ||
           parser_check(parser, TOK_LTE) || parser_check(parser, TOK_GTE)) {
        int line = parser_current(parser)->line;
        Token *op_tok = parser_advance(parser);

        ASTNode *right = parse_additive(parser);
        if (!right) {
            ast_free(left);
            return NULL;
        }

        ASTNode *node = ast_create(NODE_BINOP, line);
        node->data.binop.op = nerd_strdup(op_tok->value);
        node->data.binop.left = left;
        node->data.binop.right = right;
        left = node;
    }

    return left;
}

/*
 * Parse and expression
 */
static ASTNode *parse_and(Parser *parser) {
    ASTNode *left = parse_comparison(parser);
    if (!left) return NULL;

    while (parser_match(parser, TOK_AND)) {
        int line = parser_current(parser)->line;

        ASTNode *right = parse_comparison(parser);
        if (!right) {
            ast_free(left);
            return NULL;
        }

        ASTNode *node = ast_create(NODE_BINOP, line);
        node->data.binop.op = nerd_strdup("and");
        node->data.binop.left = left;
        node->data.binop.right = right;
        left = node;
    }

    return left;
}

/*
 * Parse or expression
 */
static ASTNode *parse_or(Parser *parser) {
    ASTNode *left = parse_and(parser);
    if (!left) return NULL;

    while (parser_check(parser, TOK_OR) && !parser_at_end_of_line(parser)) {
        int line = parser_current(parser)->line;
        parser_advance(parser);

        ASTNode *right = parse_and(parser);
        if (!right) {
            ast_free(left);
            return NULL;
        }

        ASTNode *node = ast_create(NODE_BINOP, line);
        node->data.binop.op = nerd_strdup("or");
        node->data.binop.left = left;
        node->data.binop.right = right;
        left = node;
    }

    return left;
}

/*
 * Parse expression
 */
static ASTNode *parse_expr(Parser *parser) {
    return parse_or(parser);
}

/*
 * Parse inline statement (used after 'if condition')
 */
static ASTNode *parse_inline_stmt(Parser *parser) {
    int line = parser_current(parser)->line;

    // Return statement
    if (parser_match(parser, TOK_RET)) {
        ASTNode *node = ast_create(NODE_RETURN, line);
        node->data.ret.variant = 0;

        if (parser_match(parser, TOK_OK)) {
            node->data.ret.variant = 1;
        } else if (parser_match(parser, TOK_ERR)) {
            node->data.ret.variant = 2;
        }

        node->data.ret.value = parse_expr(parser);
        if (!node->data.ret.value) {
            ast_free(node);
            return NULL;
        }

        return node;
    }

    // Out statement
    if (parser_match(parser, TOK_OUT)) {
        ASTNode *node = ast_create(NODE_OUT, line);
        node->data.out.value = parse_expr(parser);
        if (!node->data.out.value) {
            ast_free(node);
            return NULL;
        }
        return node;
    }

    // Let binding
    if (parser_match(parser, TOK_LET)) {
        Token *name_tok = parser_expect(parser, TOK_IDENT, "Expected variable name");
        if (!name_tok) return NULL;

        ASTNode *node = ast_create(NODE_LET, line);
        node->data.let.name = nerd_strdup(name_tok->value);
        node->data.let.value = parse_expr(parser);
        if (!node->data.let.value) {
            ast_free(node);
            return NULL;
        }

        return node;
    }

    // Expression statement
    ASTNode *expr = parse_expr(parser);
    if (!expr) return NULL;

    ASTNode *node = ast_create(NODE_EXPR_STMT, line);
    node->data.expr_stmt.expr = expr;
    return node;
}

/*
 * Parse statement
 */
static ASTNode *parse_stmt(Parser *parser) {
    int line = parser_current(parser)->line;

    // Return statement
    if (parser_match(parser, TOK_RET)) {
        ASTNode *node = ast_create(NODE_RETURN, line);
        node->data.ret.variant = 0;

        if (parser_match(parser, TOK_OK)) {
            node->data.ret.variant = 1;
        } else if (parser_match(parser, TOK_ERR)) {
            node->data.ret.variant = 2;
        }

        node->data.ret.value = parse_expr(parser);
        if (!node->data.ret.value) {
            ast_free(node);
            return NULL;
        }

        parser_match(parser, TOK_NEWLINE);
        return node;
    }

    // Out statement
    if (parser_match(parser, TOK_OUT)) {
        ASTNode *node = ast_create(NODE_OUT, line);
        node->data.out.value = parse_expr(parser);
        if (!node->data.out.value) {
            ast_free(node);
            return NULL;
        }
        parser_match(parser, TOK_NEWLINE);
        return node;
    }

    // Inc statement: inc var [amount]
    if (parser_match(parser, TOK_INC)) {
        Token *var_tok = parser_expect(parser, TOK_IDENT, "Expected variable name after 'inc'");
        if (!var_tok) return NULL;

        ASTNode *node = ast_create(NODE_INC, line);
        node->data.inc.var_name = nerd_strdup(var_tok->value);
        node->data.inc.amount = NULL;

        // Optional amount
        if (!parser_at_end_of_line(parser)) {
            node->data.inc.amount = parse_expr(parser);
        }

        parser_match(parser, TOK_NEWLINE);
        return node;
    }

    // Dec statement: dec var [amount]
    if (parser_match(parser, TOK_DEC)) {
        Token *var_tok = parser_expect(parser, TOK_IDENT, "Expected variable name after 'dec'");
        if (!var_tok) return NULL;

        ASTNode *node = ast_create(NODE_DEC, line);
        node->data.dec.var_name = nerd_strdup(var_tok->value);
        node->data.dec.amount = NULL;

        // Optional amount
        if (!parser_at_end_of_line(parser)) {
            node->data.dec.amount = parse_expr(parser);
        }

        parser_match(parser, TOK_NEWLINE);
        return node;
    }

    // If statement
    if (parser_match(parser, TOK_IF)) {
        ASTNode *condition = parse_comparison(parser);
        if (!condition) return NULL;

        ASTNode *node = ast_create(NODE_IF, line);
        node->data.if_stmt.condition = condition;
        node->data.if_stmt.then_stmt = NULL;
        node->data.if_stmt.else_stmt = NULL;

        // Check if this is a multiline if (newline after condition)
        if (parser_check(parser, TOK_NEWLINE)) {
            // Multiline if block: if cond \n stmt* \n [else \n stmt*] done
            parser_match(parser, TOK_NEWLINE);
            parser_skip_newlines(parser);

            // Create a block node to hold multiple statements (reuse program node)
            ASTNode *then_block = ast_create(NODE_PROGRAM, line);
            ast_list_init(&then_block->data.program.functions);  // Reuse as statement list

            // Parse then statements until else or done
            while (!parser_at_end(parser) && 
                   !parser_check(parser, TOK_ELSE) && 
                   !parser_check(parser, TOK_DONE)) {
                if (parser_match(parser, TOK_NEWLINE)) continue;
                ASTNode *stmt = parse_stmt(parser);
                if (!stmt) {
                    ast_free(node);
                    ast_free(then_block);
                    return NULL;
                }
                ast_list_push(&then_block->data.program.functions, stmt);
                parser_skip_newlines(parser);
            }

            // For single statement, unwrap the block
            if (then_block->data.program.functions.count == 1) {
                node->data.if_stmt.then_stmt = then_block->data.program.functions.nodes[0];
                then_block->data.program.functions.nodes[0] = NULL;
                then_block->data.program.functions.count = 0;
                ast_free(then_block);
            } else {
                // Multiple statements - wrap in expression statement for now
                // TODO: proper block support
                if (then_block->data.program.functions.count > 0) {
                    node->data.if_stmt.then_stmt = then_block->data.program.functions.nodes[0];
                }
                ast_free(then_block);
            }

            // Check for else
            if (parser_match(parser, TOK_ELSE)) {
                parser_match(parser, TOK_NEWLINE);
                parser_skip_newlines(parser);

                // Check if else if
                if (parser_check(parser, TOK_IF)) {
                    node->data.if_stmt.else_stmt = parse_stmt(parser);
                } else {
                    // Parse else statements until done
                    ASTNode *else_block = ast_create(NODE_PROGRAM, line);
                    ast_list_init(&else_block->data.program.functions);

                    while (!parser_at_end(parser) && !parser_check(parser, TOK_DONE)) {
                        if (parser_match(parser, TOK_NEWLINE)) continue;
                        ASTNode *stmt = parse_stmt(parser);
                        if (!stmt) {
                            ast_free(node);
                            ast_free(else_block);
                            return NULL;
                        }
                        ast_list_push(&else_block->data.program.functions, stmt);
                        parser_skip_newlines(parser);
                    }

                    if (else_block->data.program.functions.count == 1) {
                        node->data.if_stmt.else_stmt = else_block->data.program.functions.nodes[0];
                        else_block->data.program.functions.nodes[0] = NULL;
                        else_block->data.program.functions.count = 0;
                        ast_free(else_block);
                    } else if (else_block->data.program.functions.count > 0) {
                        node->data.if_stmt.else_stmt = else_block->data.program.functions.nodes[0];
                        ast_free(else_block);
                    } else {
                        ast_free(else_block);
                    }
                }
            }

            // Expect done
            if (!parser_check(parser, TOK_IF)) {  // else if doesn't need done
                parser_expect(parser, TOK_DONE, "Expected 'done' to end if block");
                parser_match(parser, TOK_NEWLINE);
            }
        } else {
            // Inline if: if cond stmt [else stmt]
            node->data.if_stmt.then_stmt = parse_inline_stmt(parser);
            if (!node->data.if_stmt.then_stmt) {
                ast_free(node);
                return NULL;
            }

            // Check for else branch (on same line)
            if (parser_match(parser, TOK_ELSE)) {
                if (parser_check(parser, TOK_IF)) {
                    node->data.if_stmt.else_stmt = parse_stmt(parser);
                } else {
                    node->data.if_stmt.else_stmt = parse_inline_stmt(parser);
                    parser_match(parser, TOK_NEWLINE);
                }
            } else {
                parser_match(parser, TOK_NEWLINE);
            }
        }

        return node;
    }

    // Let binding
    if (parser_match(parser, TOK_LET)) {
        Token *name_tok = parser_expect(parser, TOK_IDENT, "Expected variable name");
        if (!name_tok) return NULL;

        ASTNode *node = ast_create(NODE_LET, line);
        node->data.let.name = nerd_strdup(name_tok->value);

        // Check for {} (empty JSON object)
        if (parser_check(parser, TOK_LBRACE)) {
            parser_advance(parser);  // consume {
            if (!parser_expect(parser, TOK_RBRACE, "Expected '}' after '{'")) {
                ast_free(node);
                return NULL;
            }
            // Create a JSON_NEW node
            ASTNode *json_new = ast_create(NODE_JSON_NEW, line);
            node->data.let.value = json_new;
        } else {
            node->data.let.value = parse_expr(parser);
            if (!node->data.let.value) {
                ast_free(node);
                return NULL;
            }
        }

        parser_match(parser, TOK_NEWLINE);
        return node;
    }

    // Repeat loop: repeat <n> times [as <var>] ... done
    if (parser_match(parser, TOK_REPEAT)) {
        // Parse count as a simple value (not full expression) to avoid 'times' ambiguity
        ASTNode *count = parse_primary(parser);
        if (!count) return NULL;

        // Expect 'times' keyword
        if (!parser_expect(parser, TOK_TIMES, "Expected 'times' after repeat count")) {
            ast_free(count);
            return NULL;
        }

        ASTNode *node = ast_create(NODE_REPEAT, line);
        node->data.repeat.count = count;
        node->data.repeat.var_name = NULL;
        ast_list_init(&node->data.repeat.body);

        // Optional 'as <var>'
        if (parser_match(parser, TOK_AS)) {
            Token *var_tok = parser_expect(parser, TOK_IDENT, "Expected variable name after 'as'");
            if (!var_tok) {
                ast_free(node);
                return NULL;
            }
            node->data.repeat.var_name = nerd_strdup(var_tok->value);
        }

        parser_match(parser, TOK_NEWLINE);
        parser_skip_newlines(parser);

        // Parse body until 'done'
        while (!parser_at_end(parser) && !parser_check(parser, TOK_DONE)) {
            if (parser_match(parser, TOK_NEWLINE)) continue;

            ASTNode *stmt = parse_stmt(parser);
            if (!stmt) {
                ast_free(node);
                return NULL;
            }
            ast_list_push(&node->data.repeat.body, stmt);
            parser_skip_newlines(parser);
        }

        if (!parser_expect(parser, TOK_DONE, "Expected 'done' to end repeat block")) {
            ast_free(node);
            return NULL;
        }
        parser_match(parser, TOK_NEWLINE);

        return node;
    }

    // While loop: while <cond> ... done
    if (parser_match(parser, TOK_WHILE)) {
        ASTNode *condition = parse_comparison(parser);
        if (!condition) return NULL;

        ASTNode *node = ast_create(NODE_WHILE, line);
        node->data.while_loop.condition = condition;
        ast_list_init(&node->data.while_loop.body);

        parser_match(parser, TOK_NEWLINE);
        parser_skip_newlines(parser);

        // Parse body until 'done'
        while (!parser_at_end(parser) && !parser_check(parser, TOK_DONE)) {
            if (parser_match(parser, TOK_NEWLINE)) continue;

            ASTNode *stmt = parse_stmt(parser);
            if (!stmt) {
                ast_free(node);
                return NULL;
            }
            ast_list_push(&node->data.while_loop.body, stmt);
            parser_skip_newlines(parser);
        }

        if (!parser_expect(parser, TOK_DONE, "Expected 'done' to end while block")) {
            ast_free(node);
            return NULL;
        }
        parser_match(parser, TOK_NEWLINE);

        return node;
    }

    // JSON set statement: identifier."key" = value
    // Check if this looks like a JSON set (identifier followed by . and string)
    if (parser_check(parser, TOK_IDENT)) {
        // Peek ahead to see if it's a JSON set
        size_t saved_pos = parser->pos;
        Token *var_tok = parser_advance(parser);
        
        if (parser_check(parser, TOK_DOT)) {
            parser_advance(parser);  // consume .
            if (parser_check(parser, TOK_STRING)) {
                Token *key_tok = parser_advance(parser);
                if (parser_check(parser, TOK_ASSIGN)) {
                    parser_advance(parser);  // consume =
                    
                    // This is a JSON set statement
                    ASTNode *value = parse_expr(parser);
                    if (!value) {
                        return NULL;
                    }
                    
                    ASTNode *var_node = ast_create(NODE_VAR, line);
                    var_node->data.var.name = nerd_strdup(var_tok->value);
                    
                    ASTNode *node = ast_create(NODE_JSON_SET, line);
                    node->data.json_set.object = var_node;
                    node->data.json_set.key = nerd_strdup(key_tok->value);
                    node->data.json_set.value = value;
                    
                    parser_match(parser, TOK_NEWLINE);
                    return node;
                }
            }
        }
        
        // Restore position - it's not a JSON set
        parser->pos = saved_pos;
    }

    // Expression statement
    ASTNode *expr = parse_expr(parser);
    if (!expr) return NULL;

    ASTNode *node = ast_create(NODE_EXPR_STMT, line);
    node->data.expr_stmt.expr = expr;
    parser_match(parser, TOK_NEWLINE);
    return node;
}

/*
 * Parse function definition
 */
static ASTNode *parse_func_def(Parser *parser) {
    int line = parser_current(parser)->line;
    parser_expect(parser, TOK_FN, "Expected 'fn'");

    Token *name_tok = parser_expect(parser, TOK_IDENT, "Expected function name");
    if (!name_tok) return NULL;

    ASTNode *node = ast_create(NODE_FUNC_DEF, line);
    node->data.func_def.name = nerd_strdup(name_tok->value);
    ast_list_init(&node->data.func_def.params);
    ast_list_init(&node->data.func_def.body);
    node->data.func_def.return_type = NULL;

    // Parse parameters
    while (!parser_at_end_of_line(parser) && parser_check(parser, TOK_IDENT)) {
        Token *param_tok = parser_advance(parser);
        ASTNode *param = ast_create(NODE_PARAM, param_tok->line);
        param->data.param.name = nerd_strdup(param_tok->value);
        param->data.param.param_type = NULL;
        ast_list_push(&node->data.func_def.params, param);
    }

    parser_match(parser, TOK_NEWLINE);
    parser_skip_newlines(parser);

    // Parse body
    while (!parser_at_end(parser) &&
           !parser_check(parser, TOK_FN) &&
           !parser_check(parser, TOK_TYPE)) {
        if (parser_match(parser, TOK_NEWLINE)) continue;

        ASTNode *stmt = parse_stmt(parser);
        if (!stmt) {
            ast_free(node);
            return NULL;
        }
        ast_list_push(&node->data.func_def.body, stmt);
        parser_skip_newlines(parser);
    }

    return node;
}

/*
 * Parse type definition
 */
static ASTNode *parse_type_def(Parser *parser) {
    int line = parser_current(parser)->line;
    parser_expect(parser, TOK_TYPE, "Expected 'type'");

    Token *name_tok = parser_expect(parser, TOK_IDENT, "Expected type name");
    if (!name_tok) return NULL;

    ASTNode *node = ast_create(NODE_TYPE_DEF, line);
    node->data.type_def.name = nerd_strdup(name_tok->value);
    node->data.type_def.is_union = false;
    ast_list_init(&node->data.type_def.fields);
    node->data.type_def.ok_type = NULL;
    node->data.type_def.err_type = NULL;

    // Check for union type: ok type or err type
    if (parser_match(parser, TOK_OK)) {
        node->data.type_def.is_union = true;
        // Parse ok type (simplified - just check for type token)
        if (is_type_token(parser)) {
            parser_advance(parser);
        }
        parser_expect(parser, TOK_OR, "Expected 'or' in union type");
        parser_expect(parser, TOK_ERR, "Expected 'err' in union type");
        if (is_type_token(parser)) {
            parser_advance(parser);
        }
    } else {
        // Struct type: fields until end of line
        while (!parser_at_end_of_line(parser)) {
            if (is_type_token(parser) || parser_check(parser, TOK_IDENT)) {
                parser_advance(parser);
                // Just skip for now - we'll handle types properly later
            } else {
                break;
            }
        }
    }

    parser_match(parser, TOK_NEWLINE);
    return node;
}

/*
 * Parse program (supports implicit main)
 */
ASTNode *parser_parse(Parser *parser) {
    ASTNode *program = ast_create(NODE_PROGRAM, 1);
    ast_list_init(&program->data.program.types);
    ast_list_init(&program->data.program.functions);

    // Collect top-level statements for implicit main
    ASTList top_level_stmts;
    ast_list_init(&top_level_stmts);
    bool has_explicit_main = false;

    parser_skip_newlines(parser);

    while (!parser_at_end(parser)) {
        if (parser_check(parser, TOK_TYPE)) {
            ASTNode *type_def = parse_type_def(parser);
            if (!type_def) {
                ast_list_free(&top_level_stmts);
                ast_free(program);
                return NULL;
            }
            ast_list_push(&program->data.program.types, type_def);
        } else if (parser_check(parser, TOK_FN)) {
            ASTNode *func_def = parse_func_def(parser);
            if (!func_def) {
                ast_list_free(&top_level_stmts);
                ast_free(program);
                return NULL;
            }
            if (strcmp(func_def->data.func_def.name, "main") == 0) {
                has_explicit_main = true;
            }
            ast_list_push(&program->data.program.functions, func_def);
        } else if (parser_match(parser, TOK_NEWLINE)) {
            continue;
        } else {
            // Top-level statement -> implicit main
            ASTNode *stmt = parse_stmt(parser);
            if (!stmt) {
                ast_list_free(&top_level_stmts);
                ast_free(program);
                return NULL;
            }
            ast_list_push(&top_level_stmts, stmt);
        }

        parser_skip_newlines(parser);
    }

    // Create implicit main if needed
    if (top_level_stmts.count > 0 && !has_explicit_main) {
        ASTNode *implicit_main = ast_create(NODE_FUNC_DEF, 1);
        implicit_main->data.func_def.name = nerd_strdup("main");
        ast_list_init(&implicit_main->data.func_def.params);
        implicit_main->data.func_def.return_type = NULL;
        implicit_main->data.func_def.body = top_level_stmts;
        ast_list_push(&program->data.program.functions, implicit_main);
    } else if (top_level_stmts.count > 0 && has_explicit_main) {
        fprintf(stderr, "Error: Cannot mix top-level statements with explicit main\n");
        ast_list_free(&top_level_stmts);
        ast_free(program);
        return NULL;
    }

    return program;
}
