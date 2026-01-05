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
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
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
        case TOK_ELSE: return "ELSE";
        case TOK_OR: return "OR";
        case TOK_OK: return "OK";
        case TOK_ERR: return "ERR";
        case TOK_LET: return "LET";
        case TOK_CALL: return "CALL";
        case TOK_OUT: return "OUT";
        case TOK_DONE: return "DONE";
        case TOK_REPEAT: return "REPEAT";
        case TOK_AS: return "AS";
        case TOK_WHILE: return "WHILE";
        case TOK_NEG: return "NEG";
        case TOK_INC: return "INC";
        case TOK_DEC: return "DEC";
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
        case TOK_MCP: return "MCP";
        case TOK_LLM: return "LLM";
        case TOK_GET: return "GET";
        case TOK_POST: return "POST";
        case TOK_PUT: return "PUT";
        case TOK_DELETE: return "DELETE";
        case TOK_PATCH: return "PATCH";
        case TOK_WITH: return "WITH";
        case TOK_AUTH: return "AUTH";
        case TOK_BEARER: return "BEARER";
        case TOK_BASIC: return "BASIC";
        case TOK_COUNT: return "COUNT";
        case TOK_LBRACE: return "LBRACE";
        case TOK_RBRACE: return "RBRACE";
        case TOK_DOT: return "DOT";
        case TOK_QUESTION: return "QUESTION";
        case TOK_ASSIGN: return "ASSIGN";
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

        case NODE_OUT:
            printf("Out\n");
            print_ast(node->data.out.value, indent + 1);
            break;

        case NODE_REPEAT:
            printf("Repeat %s\n", node->data.repeat.var_name ? node->data.repeat.var_name : "(no var)");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Count:\n");
            print_ast(node->data.repeat.count, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Body:\n");
            for (size_t i = 0; i < node->data.repeat.body.count; i++) {
                print_ast(node->data.repeat.body.nodes[i], indent + 2);
            }
            break;

        case NODE_WHILE:
            printf("While\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Condition:\n");
            print_ast(node->data.while_loop.condition, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Body:\n");
            for (size_t i = 0; i < node->data.while_loop.body.count; i++) {
                print_ast(node->data.while_loop.body.nodes[i], indent + 2);
            }
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
    printf("  nerd run <file.nerd>                      Compile and run\n");
    printf("  nerd compile <file.nerd> [-o output.ll]   Compile to LLVM IR\n");
    printf("  nerd parse <file.nerd>                    Parse and dump AST\n");
    printf("  nerd tokens <file.nerd>                   Show tokens\n");
    printf("  nerd --version                            Show version\n");
    printf("  nerd --help                               Show this help\n");
    printf("\n");
    printf("Examples:\n");
    printf("  nerd run math.nerd\n");
    printf("  nerd compile math.nerd -o math.ll\n");
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
 * Run command - compile and execute
 */
static int cmd_run(int argc, char **argv) {
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

    // Check which modules are used
    bool needs_http = false, needs_mcp = false, needs_llm = false;
    for (size_t i = 0; i < lexer->token_count; i++) {
        if (lexer->tokens[i].type == TOK_HTTP) needs_http = true;
        if (lexer->tokens[i].type == TOK_MCP) needs_mcp = true;
        if (lexer->tokens[i].type == TOK_LLM) needs_llm = true;
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

    // Generate code to temp file
    const char *tmp_ll = "/tmp/nerd_out.ll";
    const char *tmp_main = "/tmp/nerd_main.ll";
    const char *tmp_combined = "/tmp/nerd_combined.ll";
    const char *tmp_bin = "/tmp/nerd_run";

    NerdContext ctx = {0};
    ctx.filename = input_file;
    ctx.source = source;
    ctx.ast = ast;

    if (!codegen_llvm(&ctx, tmp_ll)) {
        fprintf(stderr, "Error: %s\n", ctx.error_msg);
        ast_free(ast);
        parser_free(parser);
        lexer_free(lexer);
        free(source);
        return 1;
    }

    // Check if there's a main function in the AST
    ASTNode *program = ast;
    bool has_main = false;
    for (size_t i = 0; i < program->data.program.functions.count; i++) {
        ASTNode *func = program->data.program.functions.nodes[i];
        if (strcmp(func->data.func_def.name, "main") == 0) {
            has_main = true;
            break;
        }
    }

    char cmd[2048];
    
    if (has_main) {
        // Program has main - create i32 wrapper that calls nerd's double main
        // Rename the NERD main to nerd_main, then create i32 main wrapper
        snprintf(cmd, sizeof(cmd), 
            "sed 's/define double @main/define double @nerd_main/g' %s > %s",
            tmp_ll, tmp_combined);
        if (system(cmd) != 0) {
            fprintf(stderr, "Error: Failed to process file\n");
            ast_free(ast); parser_free(parser); lexer_free(lexer); free(source);
            return 1;
        }
        
        // Append i32 main wrapper
        FILE *f = fopen(tmp_combined, "a");
        if (f) {
            fprintf(f, "\n; Entry point wrapper\n");
            fprintf(f, "define i32 @main() {\n");
            fprintf(f, "entry:\n");
            fprintf(f, "  call double @nerd_main()\n");
            fprintf(f, "  ret i32 0\n");
            fprintf(f, "}\n");
            fclose(f);
        }
    } else {
        // No main - generate test wrapper (old behavior for library-style code)
        FILE *main_file = fopen(tmp_main, "w");
        if (!main_file) {
            fprintf(stderr, "Error: Cannot create temp file\n");
            ast_free(ast); parser_free(parser); lexer_free(lexer); free(source);
            return 1;
        }

        fprintf(main_file, "; Auto-generated main for nerd run\n\n");
        fprintf(main_file, "@.fmt = private constant [11 x i8] c\"%%s = %%.0f\\0A\\00\"\n");
        fprintf(main_file, "declare i32 @printf(i8*, ...)\n\n");

        size_t func_count = program->data.program.functions.count;
        for (size_t i = 0; i < func_count; i++) {
            ASTNode *func = program->data.program.functions.nodes[i];
            const char *name = func->data.func_def.name;
            fprintf(main_file, "@.name%zu = private constant [%zu x i8] c\"%s\\00\"\n", 
                    i, strlen(name) + 1, name);
        }

        fprintf(main_file, "\ndefine i32 @main() {\n");
        fprintf(main_file, "entry:\n");

        for (size_t i = 0; i < func_count; i++) {
            ASTNode *func = program->data.program.functions.nodes[i];
            const char *name = func->data.func_def.name;
            size_t param_count = func->data.func_def.params.count;
            
            fprintf(main_file, "  %%r%zu = call double @%s(", i, name);
            for (size_t j = 0; j < param_count; j++) {
                if (j > 0) fprintf(main_file, ", ");
                if (j == 0) fprintf(main_file, "double 5.0");
                else if (j == 1) fprintf(main_file, "double 3.0");
                else fprintf(main_file, "double 1.0");
            }
            fprintf(main_file, ")\n");
            
            fprintf(main_file, "  %%fmt%zu = getelementptr [11 x i8], [11 x i8]* @.fmt, i32 0, i32 0\n", i);
            fprintf(main_file, "  %%nm%zu = getelementptr [%zu x i8], [%zu x i8]* @.name%zu, i32 0, i32 0\n", 
                    i, strlen(name) + 1, strlen(name) + 1, i);
            fprintf(main_file, "  call i32 (i8*, ...) @printf(i8* %%fmt%zu, i8* %%nm%zu, double %%r%zu)\n", i, i, i);
        }

        fprintf(main_file, "  ret i32 0\n");
        fprintf(main_file, "}\n");
        fclose(main_file);

        snprintf(cmd, sizeof(cmd), "cat %s %s > %s", tmp_ll, tmp_main, tmp_combined);
        if (system(cmd) != 0) {
            fprintf(stderr, "Error: Failed to combine files\n");
            ast_free(ast); parser_free(parser); lexer_free(lexer); free(source);
            return 1;
        }
    }

    // Build clang command with required libraries
    // Get path to nerd executable to find runtime libs
    char exe_path[1024] = "";
    #ifdef __APPLE__
    uint32_t size = sizeof(exe_path);
    _NSGetExecutablePath(exe_path, &size);
    #else
    readlink("/proc/self/exe", exe_path, sizeof(exe_path));
    #endif
    
    // Get directory of executable
    char *last_slash = strrchr(exe_path, '/');
    if (last_slash) *(last_slash + 1) = '\0';
    
    // Determine runtime lib path: try lib/ first (release), then build/ (dev)
    char lib_path[1024];
    char test_path[1024];
    snprintf(test_path, sizeof(test_path), "%slib/cJSON.o", exe_path);
    if (access(test_path, F_OK) == 0) {
        // Release mode: libs in lib/ subfolder
        snprintf(lib_path, sizeof(lib_path), "%slib/", exe_path);
    } else {
        // Dev mode: libs in build/ subfolder
        snprintf(lib_path, sizeof(lib_path), "%sbuild/", exe_path);
    }
    
    // Build library paths
    char cjson_lib[1024], json_lib[1024], http_lib[1024], mcp_lib[1024], llm_lib[1024];
    snprintf(cjson_lib, sizeof(cjson_lib), "%scJSON.o", lib_path);
    snprintf(json_lib, sizeof(json_lib), "%snerd_json.o", lib_path);
    snprintf(http_lib, sizeof(http_lib), "%snerd_http.o", lib_path);
    snprintf(mcp_lib, sizeof(mcp_lib), "%snerd_mcp.o", lib_path);
    snprintf(llm_lib, sizeof(llm_lib), "%snerd_llm.o", lib_path);
    
    // Build clang command
    char libs[4096] = "";
    if (needs_http || needs_mcp || needs_llm) {
        strcat(libs, " -lcurl");
    }
    // JSON support is needed for HTTP (auto-parsing)
    if (needs_http) {
        strcat(libs, " ");
        strcat(libs, cjson_lib);
        strcat(libs, " ");
        strcat(libs, json_lib);
        strcat(libs, " ");
        strcat(libs, http_lib);
    }
    if (needs_mcp) {
        strcat(libs, " ");
        strcat(libs, mcp_lib);
    }
    if (needs_llm) {
        strcat(libs, " ");
        strcat(libs, llm_lib);
    }
    
    snprintf(cmd, sizeof(cmd), "clang -w %s%s -o %s", tmp_combined, libs, tmp_bin);
    if (system(cmd) != 0) {
        fprintf(stderr, "Error: clang compilation failed. Check %s\n", tmp_combined);
        ast_free(ast);
        parser_free(parser);
        lexer_free(lexer);
        free(source);
        return 1;
    }

    // Run
    int result = system(tmp_bin);

    // Cleanup
    remove(tmp_ll);
    remove(tmp_main);
    remove(tmp_combined);
    remove(tmp_bin);

    ast_free(ast);
    parser_free(parser);
    lexer_free(lexer);
    free(source);

    return result;
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

    if (strcmp(cmd, "run") == 0) {
        return cmd_run(argc - 2, argv + 2);
    } else if (strcmp(cmd, "compile") == 0) {
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
