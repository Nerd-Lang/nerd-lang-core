/*
 * NERD Bootstrap Compiler - Main Entry Point
 *
 * Usage:
 *   nerd compile <file.nerd> [-o output]    Compile to LLVM IR / native
 *   nerd run <file.nerd> [args...]          Compile and run
 *   nerd parse <file.nerd>                  Parse and dump AST
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nerd.h"

/*
 * Read entire file into memory
 */
static char *read_file(const char *path, size_t *len) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    *len = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = malloc(*len + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    size_t read = fread(buf, 1, *len, f);
    if (read != *len) {
        fprintf(stderr, "Error: Failed to read file '%s'\n", path);
        free(buf);
        fclose(f);
        return NULL;
    }
    buf[*len] = '\0';
    fclose(f);

    return buf;
}

/*
 * Print token for debugging
 */
static const char *token_name(TokenType type) {
    switch (type) {
        case TOK_FN: return "FN";
        case TOK_RET: return "RET";
        case TOK_TYPE: return "TYPE";
        case TOK_IF: return "IF";
        case TOK_OR: return "OR";
        case TOK_OK: return "OK";
        case TOK_ERR: return "ERR";
        case TOK_LET: return "LET";
        case TOK_CALL: return "CALL";
        case TOK_NUM: return "NUM";
        case TOK_INT: return "INT";
        case TOK_STR: return "STR";
        case TOK_BOOL: return "BOOL";
        case TOK_VOID: return "VOID";
        case TOK_PLUS: return "PLUS";
        case TOK_MINUS: return "MINUS";
        case TOK_TIMES: return "TIMES";
        case TOK_OVER: return "OVER";
        case TOK_MOD: return "MOD";
        case TOK_EQ: return "EQ";
        case TOK_NEQ: return "NEQ";
        case TOK_LT: return "LT";
        case TOK_GT: return "GT";
        case TOK_LTE: return "LTE";
        case TOK_GTE: return "GTE";
        case TOK_AND: return "AND";
        case TOK_NOT: return "NOT";
        case TOK_FIRST: return "FIRST";
        case TOK_SECOND: return "SECOND";
        case TOK_THIRD: return "THIRD";
        case TOK_FOURTH: return "FOURTH";
        case TOK_ZERO: return "ZERO";
        case TOK_ONE: return "ONE";
        case TOK_TWO: return "TWO";
        case TOK_THREE: return "THREE";
        case TOK_FOUR: return "FOUR";
        case TOK_FIVE: return "FIVE";
        case TOK_SIX: return "SIX";
        case TOK_SEVEN: return "SEVEN";
        case TOK_EIGHT: return "EIGHT";
        case TOK_NINE: return "NINE";
        case TOK_TEN: return "TEN";
        case TOK_MATH: return "MATH";
        case TOK_LIST: return "LIST";
        case TOK_TIME: return "TIME";
        case TOK_HTTP: return "HTTP";
        case TOK_JSON: return "JSON";
        case TOK_NUMBER: return "NUMBER";
        case TOK_STRING: return "STRING";
        case TOK_IDENT: return "IDENT";
        case TOK_NEWLINE: return "NEWLINE";
        case TOK_EOF: return "EOF";
        default: return "UNKNOWN";
    }
}

/*
 * Print AST node for debugging
 */
static void print_ast(ASTNode *node, int indent) {
    if (!node) return;

    for (int i = 0; i < indent; i++) printf("  ");

    switch (node->type) {
        case NODE_PROGRAM:
            printf("Program\n");
            for (size_t i = 0; i < node->data.program.types.count; i++) {
                print_ast(node->data.program.types.nodes[i], indent + 1);
            }
            for (size_t i = 0; i < node->data.program.functions.count; i++) {
                print_ast(node->data.program.functions.nodes[i], indent + 1);
            }
            break;

        case NODE_FUNC_DEF:
            printf("Function: %s (", node->data.func_def.name);
            for (size_t i = 0; i < node->data.func_def.params.count; i++) {
                if (i > 0) printf(", ");
                printf("%s", node->data.func_def.params.nodes[i]->data.param.name);
            }
            printf(")\n");
            for (size_t i = 0; i < node->data.func_def.body.count; i++) {
                print_ast(node->data.func_def.body.nodes[i], indent + 1);
            }
            break;

        case NODE_TYPE_DEF:
            printf("Type: %s (%s)\n", node->data.type_def.name,
                   node->data.type_def.is_union ? "union" : "struct");
            break;

        case NODE_RETURN:
            printf("Return");
            if (node->data.ret.variant == 1) printf(" ok");
            else if (node->data.ret.variant == 2) printf(" err");
            printf("\n");
            print_ast(node->data.ret.value, indent + 1);
            break;

        case NODE_IF:
            printf("If\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Condition:\n");
            print_ast(node->data.if_stmt.condition, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Then:\n");
            print_ast(node->data.if_stmt.then_stmt, indent + 2);
            break;

        case NODE_LET:
            printf("Let: %s\n", node->data.let.name);
            print_ast(node->data.let.value, indent + 1);
            break;

        case NODE_EXPR_STMT:
            printf("ExprStmt\n");
            print_ast(node->data.expr_stmt.expr, indent + 1);
            break;

        case NODE_BINOP:
            printf("BinOp: %s\n", node->data.binop.op);
            print_ast(node->data.binop.left, indent + 1);
            print_ast(node->data.binop.right, indent + 1);
            break;

        case NODE_UNARYOP:
            printf("UnaryOp: %s\n", node->data.unaryop.op);
            print_ast(node->data.unaryop.operand, indent + 1);
            break;

        case NODE_CALL:
            printf("Call: %s.%s\n",
                   node->data.call.module ? node->data.call.module : "",
                   node->data.call.func);
            for (size_t i = 0; i < node->data.call.args.count; i++) {
                print_ast(node->data.call.args.nodes[i], indent + 1);
            }
            break;

        case NODE_NUM:
            printf("Num: %g\n", node->data.num.value);
            break;

        case NODE_STR:
            printf("Str: \"%s\"\n", node->data.str.value);
            break;

        case NODE_BOOL:
            printf("Bool: %s\n", node->data.boolean.value ? "true" : "false");
            break;

        case NODE_VAR:
            printf("Var: %s\n", node->data.var.name);
            break;

        case NODE_POSITIONAL:
            printf("Positional: %d\n", node->data.positional.index);
            break;

        default:
            printf("Unknown node type %d\n", node->type);
            break;
    }
}

#define NERD_VERSION "3.0.0"

/*
 * Print version
 */
static void print_version(void) {
    printf("nerd %s\n", NERD_VERSION);
}

/*
 * Print usage
 */
static void print_usage(void) {
    printf("NERD Compiler v%s - No Effort Required, Done\n", NERD_VERSION);
    printf("\n");
    printf("Usage:\n");
    printf("  nerd compile <file.nerd> [-o output.ll]   Compile to LLVM IR\n");
    printf("  nerd parse <file.nerd>                    Parse and dump AST\n");
    printf("  nerd tokens <file.nerd>                   Show tokens\n");
    printf("  nerd --version                            Show version\n");
    printf("  nerd --help                               Show this help\n");
    printf("\n");
    printf("Examples:\n");
    printf("  nerd compile math.nerd -o math.ll\n");
    printf("  clang math.ll -o math                     # Link to native\n");
    printf("  ./math\n");
}

/*
 * Compile command
 */
static int cmd_compile(int argc, char **argv) {
    const char *input_file = NULL;
    const char *output_file = NULL;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (argv[i][0] != '-') {
            input_file = argv[i];
        }
    }

    if (!input_file) {
        fprintf(stderr, "Error: No input file specified\n");
        return 1;
    }

    // Default output file
    char default_output[256];
    if (!output_file) {
        snprintf(default_output, sizeof(default_output), "%s", input_file);
        char *dot = strrchr(default_output, '.');
        if (dot) *dot = '\0';
        strcat(default_output, ".ll");
        output_file = default_output;
    }

    // Read source
    size_t source_len;
    char *source = read_file(input_file, &source_len);
    if (!source) return 1;

    // Lex
    Lexer *lexer = lexer_create(source, source_len);
    if (!lexer || !lexer_tokenize(lexer)) {
        free(source);
        return 1;
    }

    // Parse
    Parser *parser = parser_create(lexer->tokens, lexer->token_count);
    if (!parser) {
        lexer_free(lexer);
        free(source);
        return 1;
    }

    ASTNode *ast = parser_parse(parser);
    if (!ast) {
        parser_free(parser);
        lexer_free(lexer);
        free(source);
        return 1;
    }

    // Generate code
    NerdContext ctx = {0};
    ctx.filename = input_file;
    ctx.source = source;
    ctx.ast = ast;

    if (!codegen_llvm(&ctx, output_file)) {
        fprintf(stderr, "Error: %s\n", ctx.error_msg);
        ast_free(ast);
        parser_free(parser);
        lexer_free(lexer);
        free(source);
        return 1;
    }

    printf("Compiled %s -> %s\n", input_file, output_file);

    // Cleanup
    ast_free(ast);
    parser_free(parser);
    lexer_free(lexer);
    free(source);

    return 0;
}

/*
 * Parse command (show AST)
 */
static int cmd_parse(int argc, char **argv) {
    const char *input_file = NULL;

    for (int i = 0; i < argc; i++) {
        if (argv[i][0] != '-') {
            input_file = argv[i];
            break;
        }
    }

    if (!input_file) {
        fprintf(stderr, "Error: No input file specified\n");
        return 1;
    }

    // Read source
    size_t source_len;
    char *source = read_file(input_file, &source_len);
    if (!source) return 1;

    // Lex
    Lexer *lexer = lexer_create(source, source_len);
    if (!lexer || !lexer_tokenize(lexer)) {
        free(source);
        return 1;
    }

    // Parse
    Parser *parser = parser_create(lexer->tokens, lexer->token_count);
    if (!parser) {
        lexer_free(lexer);
        free(source);
        return 1;
    }

    ASTNode *ast = parser_parse(parser);
    if (!ast) {
        parser_free(parser);
        lexer_free(lexer);
        free(source);
        return 1;
    }

    // Print AST
    printf("=== AST ===\n");
    print_ast(ast, 0);

    // Cleanup
    ast_free(ast);
    parser_free(parser);
    lexer_free(lexer);
    free(source);

    return 0;
}

/*
 * Tokens command (show tokens)
 */
static int cmd_tokens(int argc, char **argv) {
    const char *input_file = NULL;

    for (int i = 0; i < argc; i++) {
        if (argv[i][0] != '-') {
            input_file = argv[i];
            break;
        }
    }

    if (!input_file) {
        fprintf(stderr, "Error: No input file specified\n");
        return 1;
    }

    // Read source
    size_t source_len;
    char *source = read_file(input_file, &source_len);
    if (!source) return 1;

    // Lex
    Lexer *lexer = lexer_create(source, source_len);
    if (!lexer || !lexer_tokenize(lexer)) {
        free(source);
        return 1;
    }

    // Print tokens
    printf("=== Tokens ===\n");
    for (size_t i = 0; i < lexer->token_count; i++) {
        Token *tok = &lexer->tokens[i];
        if (tok->type == TOK_NEWLINE) continue;
        printf("%s(%s) ", token_name(tok->type), tok->value);
    }
    printf("\n");

    // Cleanup
    lexer_free(lexer);
    free(source);

    return 0;
}

/*
 * Main entry point
 */
int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "compile") == 0) {
        return cmd_compile(argc - 2, argv + 2);
    } else if (strcmp(cmd, "parse") == 0) {
        return cmd_parse(argc - 2, argv + 2);
    } else if (strcmp(cmd, "tokens") == 0) {
        return cmd_tokens(argc - 2, argv + 2);
    } else if (strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        print_usage();
        return 0;
    } else if (strcmp(cmd, "--version") == 0 || strcmp(cmd, "-v") == 0 || strcmp(cmd, "-V") == 0) {
        print_version();
        return 0;
    } else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        print_usage();
        return 1;
    }
}
