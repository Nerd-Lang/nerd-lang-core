/*
 * NERD Code Generator - Generates LLVM IR
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nerd.h"

/*
 * Code generator state
 */
typedef struct {
    FILE *out;
    int temp_counter;
    int label_counter;
    int string_counter;

    // Current function context
    ASTNode *current_func;
    char **param_names;
    size_t param_count;

    // Local variables (doubles)
    char **local_names;
    int *local_regs;
    size_t local_count;
    size_t local_capacity;

    // Pointer locals (JSON objects, strings)
    char **ptr_local_names;
    int *ptr_local_regs;
    size_t ptr_local_count;
    size_t ptr_local_capacity;

    // String literals (deferred output)
    char **string_literals;
    size_t string_count;
    size_t string_capacity;
} CodeGen;

/*
 * Create code generator
 */
static CodeGen *codegen_create(FILE *out) {
    CodeGen *cg = calloc(1, sizeof(CodeGen));
    if (!cg) return NULL;

    cg->out = out;
    cg->temp_counter = 0;
    cg->label_counter = 0;
    cg->string_counter = 0;
    cg->local_capacity = 16;
    cg->local_names = malloc(sizeof(char*) * cg->local_capacity);
    cg->local_regs = malloc(sizeof(int) * cg->local_capacity);
    cg->ptr_local_capacity = 16;
    cg->ptr_local_names = malloc(sizeof(char*) * cg->ptr_local_capacity);
    cg->ptr_local_regs = malloc(sizeof(int) * cg->ptr_local_capacity);
    cg->string_capacity = 16;
    cg->string_literals = malloc(sizeof(char*) * cg->string_capacity);
    cg->string_count = 0;

    return cg;
}

static void codegen_free(CodeGen *cg) {
    if (!cg) return;
    for (size_t i = 0; i < cg->local_count; i++) {
        free(cg->local_names[i]);
    }
    free(cg->local_names);
    free(cg->local_regs);
    for (size_t i = 0; i < cg->ptr_local_count; i++) {
        free(cg->ptr_local_names[i]);
    }
    free(cg->ptr_local_names);
    free(cg->ptr_local_regs);
    for (size_t i = 0; i < cg->string_count; i++) {
        free(cg->string_literals[i]);
    }
    free(cg->string_literals);
    free(cg);
}

/*
 * Get actual string length after processing escape sequences
 */
static size_t actual_string_len(const char *s) {
    size_t src_len = strlen(s);
    size_t actual_len = 0;
    for (size_t j = 0; j < src_len; j++) {
        if (s[j] == '\\' && j + 1 < src_len) {
            j++;  // Skip escape sequence, counts as 1 char
        }
        actual_len++;
    }
    return actual_len;
}

/*
 * Get next temp register
 */
static int next_temp(CodeGen *cg) {
    return cg->temp_counter++;
}

/*
 * Get next label
 */
static int next_label(CodeGen *cg) {
    return cg->label_counter++;
}

/*
 * Add local variable
 */
static void add_local(CodeGen *cg, const char *name, int reg) {
    if (cg->local_count >= cg->local_capacity) {
        cg->local_capacity *= 2;
        cg->local_names = realloc(cg->local_names, sizeof(char*) * cg->local_capacity);
        cg->local_regs = realloc(cg->local_regs, sizeof(int) * cg->local_capacity);
    }
    cg->local_names[cg->local_count] = nerd_strdup(name);
    cg->local_regs[cg->local_count] = reg;
    cg->local_count++;
}

/*
 * Find local variable
 */
static int find_local(CodeGen *cg, const char *name) {
    for (size_t i = 0; i < cg->local_count; i++) {
        if (strcmp(cg->local_names[i], name) == 0) {
            return cg->local_regs[i];
        }
    }
    return -1;
}

/*
 * Add pointer local variable (for JSON objects)
 */
static void add_ptr_local(CodeGen *cg, const char *name, int reg) {
    if (cg->ptr_local_count >= cg->ptr_local_capacity) {
        cg->ptr_local_capacity *= 2;
        cg->ptr_local_names = realloc(cg->ptr_local_names, sizeof(char*) * cg->ptr_local_capacity);
        cg->ptr_local_regs = realloc(cg->ptr_local_regs, sizeof(int) * cg->ptr_local_capacity);
    }
    cg->ptr_local_names[cg->ptr_local_count] = nerd_strdup(name);
    cg->ptr_local_regs[cg->ptr_local_count] = reg;
    cg->ptr_local_count++;
}

/*
 * Find pointer local variable
 */
static int find_ptr_local(CodeGen *cg, const char *name) {
    for (size_t i = 0; i < cg->ptr_local_count; i++) {
        if (strcmp(cg->ptr_local_names[i], name) == 0) {
            return cg->ptr_local_regs[i];
        }
    }
    return -1;
}

/*
 * Find parameter index
 */
static int find_param(CodeGen *cg, const char *name) {
    for (size_t i = 0; i < cg->param_count; i++) {
        if (strcmp(cg->param_names[i], name) == 0) {
            return (int)i;
        }
    }
    return -1;
}

/*
 * Clear locals (for new function)
 */
static void clear_locals(CodeGen *cg) {
    for (size_t i = 0; i < cg->local_count; i++) {
        free(cg->local_names[i]);
    }
    cg->local_count = 0;
    for (size_t i = 0; i < cg->ptr_local_count; i++) {
        free(cg->ptr_local_names[i]);
    }
    cg->ptr_local_count = 0;
    cg->temp_counter = 0;
}

/*
 * Forward declaration
 */
static int codegen_expr(CodeGen *cg, ASTNode *node);
static void codegen_stmt(CodeGen *cg, ASTNode *node, int *result_reg);

/*
 * Collect string literals from AST (recursive)
 */
static void collect_strings_expr(CodeGen *cg, ASTNode *node);
static void collect_strings_stmt(CodeGen *cg, ASTNode *node);

static void add_string_literal(CodeGen *cg, const char *str) {
    if (cg->string_count >= cg->string_capacity) {
        cg->string_capacity *= 2;
        cg->string_literals = realloc(cg->string_literals,
            sizeof(char*) * cg->string_capacity);
    }
    cg->string_literals[cg->string_count++] = nerd_strdup(str);
}

static void collect_strings_expr(CodeGen *cg, ASTNode *node) {
    if (!node) return;

    if (node->type == NODE_STR) {
        add_string_literal(cg, node->data.str.value);
    } else if (node->type == NODE_BINOP) {
        collect_strings_expr(cg, node->data.binop.left);
        collect_strings_expr(cg, node->data.binop.right);
    } else if (node->type == NODE_UNARYOP) {
        collect_strings_expr(cg, node->data.unaryop.operand);
    } else if (node->type == NODE_CALL) {
        for (size_t i = 0; i < node->data.call.args.count; i++) {
            collect_strings_expr(cg, node->data.call.args.nodes[i]);
        }
    } else if (node->type == NODE_JSON_ACCESS) {
        add_string_literal(cg, node->data.json_access.path);
        collect_strings_expr(cg, node->data.json_access.object);
    } else if (node->type == NODE_JSON_HAS) {
        add_string_literal(cg, node->data.json_has.path);
        collect_strings_expr(cg, node->data.json_has.object);
    } else if (node->type == NODE_JSON_COUNT) {
        if (node->data.json_count.path) {
            add_string_literal(cg, node->data.json_count.path);
        }
        collect_strings_expr(cg, node->data.json_count.object);
    }
}

static void collect_strings_stmt(CodeGen *cg, ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_OUT:
            collect_strings_expr(cg, node->data.out.value);
            break;
        case NODE_RETURN:
            collect_strings_expr(cg, node->data.ret.value);
            break;
        case NODE_IF:
            collect_strings_expr(cg, node->data.if_stmt.condition);
            collect_strings_stmt(cg, node->data.if_stmt.then_stmt);
            collect_strings_stmt(cg, node->data.if_stmt.else_stmt);
            break;
        case NODE_LET:
            collect_strings_expr(cg, node->data.let.value);
            break;
        case NODE_EXPR_STMT:
            collect_strings_expr(cg, node->data.expr_stmt.expr);
            break;
        case NODE_REPEAT:
            collect_strings_expr(cg, node->data.repeat.count);
            for (size_t i = 0; i < node->data.repeat.body.count; i++) {
                collect_strings_stmt(cg, node->data.repeat.body.nodes[i]);
            }
            break;
        case NODE_WHILE:
            collect_strings_expr(cg, node->data.while_loop.condition);
            for (size_t i = 0; i < node->data.while_loop.body.count; i++) {
                collect_strings_stmt(cg, node->data.while_loop.body.nodes[i]);
            }
            break;
        case NODE_JSON_SET:
            add_string_literal(cg, node->data.json_set.key);
            collect_strings_expr(cg, node->data.json_set.value);
            break;
        default:
            break;
    }
}

static void collect_strings_func(CodeGen *cg, ASTNode *func) {
    for (size_t i = 0; i < func->data.func_def.body.count; i++) {
        collect_strings_stmt(cg, func->data.func_def.body.nodes[i]);
    }
}

static void collect_strings(CodeGen *cg, ASTNode *program) {
    for (size_t i = 0; i < program->data.program.functions.count; i++) {
        collect_strings_func(cg, program->data.program.functions.nodes[i]);
    }
}

/*
 * Generate code for expression, returns register number
 */
static int codegen_expr(CodeGen *cg, ASTNode *node) {
    if (!node) return -1;

    switch (node->type) {
        case NODE_NUM: {
            int reg = next_temp(cg);
            double val = node->data.num.value;
            // Ensure the number has a decimal point for LLVM IR
            if (val == (long long)val && val >= -1e15 && val <= 1e15) {
                fprintf(cg->out, "  %%t%d = fadd double 0.0, %.1f\n", reg, val);
            } else {
                fprintf(cg->out, "  %%t%d = fadd double 0.0, %e\n", reg, val);
            }
            return reg;
        }

        case NODE_STR: {
            // Strings need runtime support - for now, just return 0
            int reg = next_temp(cg);
            fprintf(cg->out, "  ; string: \"%s\"\n", node->data.str.value);
            fprintf(cg->out, "  %%t%d = fadd double 0.0, 0.0\n", reg);
            return reg;
        }

        case NODE_BOOL: {
            int reg = next_temp(cg);
            fprintf(cg->out, "  %%t%d = fadd double 0.0, %d.0\n", reg, node->data.boolean.value ? 1 : 0);
            return reg;
        }

        case NODE_VAR: {
            // Check locals first
            int local_reg = find_local(cg, node->data.var.name);
            if (local_reg >= 0) {
                int reg = next_temp(cg);
                fprintf(cg->out, "  %%t%d = load double, double* %%local%d\n", reg, local_reg);
                return reg;
            }

            // Check parameters
            int param_idx = find_param(cg, node->data.var.name);
            if (param_idx >= 0) {
                // Parameters are already in registers
                int reg = next_temp(cg);
                fprintf(cg->out, "  %%t%d = fadd double 0.0, %%arg%d\n", reg, param_idx);
                return reg;
            }

            fprintf(stderr, "Error: Unknown variable '%s'\n", node->data.var.name);
            return -1;
        }

        case NODE_POSITIONAL: {
            // Positional parameter reference (first, second, etc.)
            int reg = next_temp(cg);
            fprintf(cg->out, "  %%t%d = fadd double 0.0, %%arg%d\n", reg, node->data.positional.index);
            return reg;
        }

        case NODE_BINOP: {
            int left_reg = codegen_expr(cg, node->data.binop.left);
            int right_reg = codegen_expr(cg, node->data.binop.right);
            if (left_reg < 0 || right_reg < 0) return -1;

            int result_reg = next_temp(cg);
            const char *op = node->data.binop.op;

            if (strcmp(op, "plus") == 0) {
                fprintf(cg->out, "  %%t%d = fadd double %%t%d, %%t%d\n", result_reg, left_reg, right_reg);
            } else if (strcmp(op, "minus") == 0) {
                fprintf(cg->out, "  %%t%d = fsub double %%t%d, %%t%d\n", result_reg, left_reg, right_reg);
            } else if (strcmp(op, "times") == 0) {
                fprintf(cg->out, "  %%t%d = fmul double %%t%d, %%t%d\n", result_reg, left_reg, right_reg);
            } else if (strcmp(op, "over") == 0) {
                fprintf(cg->out, "  %%t%d = fdiv double %%t%d, %%t%d\n", result_reg, left_reg, right_reg);
            } else if (strcmp(op, "mod") == 0) {
                fprintf(cg->out, "  %%t%d = frem double %%t%d, %%t%d\n", result_reg, left_reg, right_reg);
            } else if (strcmp(op, "eq") == 0) {
                int cmp_reg = next_temp(cg);
                fprintf(cg->out, "  %%t%d = fcmp oeq double %%t%d, %%t%d\n", cmp_reg, left_reg, right_reg);
                fprintf(cg->out, "  %%t%d = uitofp i1 %%t%d to double\n", result_reg, cmp_reg);
            } else if (strcmp(op, "neq") == 0) {
                int cmp_reg = next_temp(cg);
                fprintf(cg->out, "  %%t%d = fcmp one double %%t%d, %%t%d\n", cmp_reg, left_reg, right_reg);
                fprintf(cg->out, "  %%t%d = uitofp i1 %%t%d to double\n", result_reg, cmp_reg);
            } else if (strcmp(op, "lt") == 0) {
                int cmp_reg = next_temp(cg);
                fprintf(cg->out, "  %%t%d = fcmp olt double %%t%d, %%t%d\n", cmp_reg, left_reg, right_reg);
                fprintf(cg->out, "  %%t%d = uitofp i1 %%t%d to double\n", result_reg, cmp_reg);
            } else if (strcmp(op, "gt") == 0) {
                int cmp_reg = next_temp(cg);
                fprintf(cg->out, "  %%t%d = fcmp ogt double %%t%d, %%t%d\n", cmp_reg, left_reg, right_reg);
                fprintf(cg->out, "  %%t%d = uitofp i1 %%t%d to double\n", result_reg, cmp_reg);
            } else if (strcmp(op, "lte") == 0) {
                int cmp_reg = next_temp(cg);
                fprintf(cg->out, "  %%t%d = fcmp ole double %%t%d, %%t%d\n", cmp_reg, left_reg, right_reg);
                fprintf(cg->out, "  %%t%d = uitofp i1 %%t%d to double\n", result_reg, cmp_reg);
            } else if (strcmp(op, "gte") == 0) {
                int cmp_reg = next_temp(cg);
                fprintf(cg->out, "  %%t%d = fcmp oge double %%t%d, %%t%d\n", cmp_reg, left_reg, right_reg);
                fprintf(cg->out, "  %%t%d = uitofp i1 %%t%d to double\n", result_reg, cmp_reg);
            } else if (strcmp(op, "and") == 0) {
                // Logical and: both non-zero
                int left_bool = next_temp(cg);
                int right_bool = next_temp(cg);
                int and_reg = next_temp(cg);
                fprintf(cg->out, "  %%t%d = fcmp one double %%t%d, 0.0\n", left_bool, left_reg);
                fprintf(cg->out, "  %%t%d = fcmp one double %%t%d, 0.0\n", right_bool, right_reg);
                fprintf(cg->out, "  %%t%d = and i1 %%t%d, %%t%d\n", and_reg, left_bool, right_bool);
                fprintf(cg->out, "  %%t%d = uitofp i1 %%t%d to double\n", result_reg, and_reg);
            } else if (strcmp(op, "or") == 0) {
                // Logical or: either non-zero
                int left_bool = next_temp(cg);
                int right_bool = next_temp(cg);
                int or_reg = next_temp(cg);
                fprintf(cg->out, "  %%t%d = fcmp one double %%t%d, 0.0\n", left_bool, left_reg);
                fprintf(cg->out, "  %%t%d = fcmp one double %%t%d, 0.0\n", right_bool, right_reg);
                fprintf(cg->out, "  %%t%d = or i1 %%t%d, %%t%d\n", or_reg, left_bool, right_bool);
                fprintf(cg->out, "  %%t%d = uitofp i1 %%t%d to double\n", result_reg, or_reg);
            } else {
                fprintf(stderr, "Error: Unknown operator '%s'\n", op);
                return -1;
            }

            return result_reg;
        }

        case NODE_UNARYOP: {
            int operand_reg = codegen_expr(cg, node->data.unaryop.operand);
            if (operand_reg < 0) return -1;

            int result_reg = next_temp(cg);
            if (strcmp(node->data.unaryop.op, "not") == 0) {
                int bool_reg = next_temp(cg);
                fprintf(cg->out, "  %%t%d = fcmp oeq double %%t%d, 0.0\n", bool_reg, operand_reg);
                fprintf(cg->out, "  %%t%d = uitofp i1 %%t%d to double\n", result_reg, bool_reg);
            } else if (strcmp(node->data.unaryop.op, "neg") == 0) {
                // Negate: 0 - x
                fprintf(cg->out, "  %%t%d = fsub double 0.0, %%t%d\n", result_reg, operand_reg);
            }

            return result_reg;
        }

        case NODE_CALL: {
            int result_reg = next_temp(cg);

            // User-defined function call (no module)
            if (node->data.call.module == NULL) {
                fprintf(cg->out, "  ; call %s\n", node->data.call.func);

                // Evaluate all arguments first
                size_t argc = node->data.call.args.count;
                int *arg_regs = argc > 0 ? malloc(sizeof(int) * argc) : NULL;
                if (argc > 0 && !arg_regs) {
                    fprintf(stderr, "Error: Out of memory\n");
                    return -1;
                }
                for (size_t i = 0; i < argc; i++) {
                    arg_regs[i] = codegen_expr(cg, node->data.call.args.nodes[i]);
                }

                // Generate call instruction
                fprintf(cg->out, "  %%t%d = call double @%s(", result_reg, node->data.call.func);
                for (size_t i = 0; i < node->data.call.args.count; i++) {
                    if (i > 0) fprintf(cg->out, ", ");
                    fprintf(cg->out, "double %%t%d", arg_regs[i]);
                }
                fprintf(cg->out, ")\n");

                free(arg_regs);
                return result_reg;
            }

            // Module calls
            fprintf(cg->out, "  ; call %s.%s\n", node->data.call.module, node->data.call.func);

            // For math functions, we can use LLVM intrinsics
            if (strcmp(node->data.call.module, "math") == 0) {
                if (node->data.call.args.count > 0) {
                    int arg_reg = codegen_expr(cg, node->data.call.args.nodes[0]);

                    if (strcmp(node->data.call.func, "abs") == 0) {
                        fprintf(cg->out, "  %%t%d = call double @llvm.fabs.f64(double %%t%d)\n", result_reg, arg_reg);
                        return result_reg;
                    } else if (strcmp(node->data.call.func, "sqrt") == 0) {
                        fprintf(cg->out, "  %%t%d = call double @llvm.sqrt.f64(double %%t%d)\n", result_reg, arg_reg);
                        return result_reg;
                    } else if (strcmp(node->data.call.func, "floor") == 0) {
                        fprintf(cg->out, "  %%t%d = call double @llvm.floor.f64(double %%t%d)\n", result_reg, arg_reg);
                        return result_reg;
                    } else if (strcmp(node->data.call.func, "ceil") == 0) {
                        fprintf(cg->out, "  %%t%d = call double @llvm.ceil.f64(double %%t%d)\n", result_reg, arg_reg);
                        return result_reg;
                    } else if (strcmp(node->data.call.func, "sin") == 0) {
                        fprintf(cg->out, "  %%t%d = call double @llvm.sin.f64(double %%t%d)\n", result_reg, arg_reg);
                        return result_reg;
                    } else if (strcmp(node->data.call.func, "cos") == 0) {
                        fprintf(cg->out, "  %%t%d = call double @llvm.cos.f64(double %%t%d)\n", result_reg, arg_reg);
                        return result_reg;
                    }

                    if (node->data.call.args.count > 1) {
                        int arg2_reg = codegen_expr(cg, node->data.call.args.nodes[1]);
                        if (strcmp(node->data.call.func, "min") == 0) {
                            fprintf(cg->out, "  %%t%d = call double @llvm.minnum.f64(double %%t%d, double %%t%d)\n",
                                    result_reg, arg_reg, arg2_reg);
                            return result_reg;
                        } else if (strcmp(node->data.call.func, "max") == 0) {
                            fprintf(cg->out, "  %%t%d = call double @llvm.maxnum.f64(double %%t%d, double %%t%d)\n",
                                    result_reg, arg_reg, arg2_reg);
                            return result_reg;
                        } else if (strcmp(node->data.call.func, "pow") == 0) {
                            fprintf(cg->out, "  %%t%d = call double @llvm.pow.f64(double %%t%d, double %%t%d)\n",
                                    result_reg, arg_reg, arg2_reg);
                            return result_reg;
                        }
                    }
                }
            }

            // HTTP functions
            if (strcmp(node->data.call.module, "http") == 0) {
                if (node->data.call.args.count > 0) {
                    // Get URL argument (must be a string literal)
                    ASTNode *url_node = node->data.call.args.nodes[0];
                    
                    // Check for headers/auth markers
                    int has_auth_bearer = 0;
                    int has_auth_basic = 0;
                    int auth_idx = -1;  // Index of __auth_bearer__ or __auth_basic__ marker
                    int header_start = -1;  // Start index of custom headers (with keyword)
                    
                    // For GET: args = [url, header1, value1, ...] or [url, __auth_bearer__, token]
                    // For POST: args = [url, body, header1, value1, ...] or [url, body, __auth_*, ...]
                    int body_offset = (strcmp(node->data.call.func, "get") == 0 || 
                                       strcmp(node->data.call.func, "delete") == 0) ? 1 : 2;
                    
                    for (size_t i = body_offset; i < node->data.call.args.count; i++) {
                        ASTNode *arg = node->data.call.args.nodes[i];
                        if (arg->type == NODE_STR) {
                            if (strcmp(arg->data.str.value, "__auth_bearer__") == 0) {
                                has_auth_bearer = 1;
                                auth_idx = (int)i;
                                break;
                            } else if (strcmp(arg->data.str.value, "__auth_basic__") == 0) {
                                has_auth_basic = 1;
                                auth_idx = (int)i;
                                break;
                            } else if (header_start < 0) {
                                // First non-auth string after URL/body is start of headers
                                header_start = (int)i;
                            }
                        }
                    }

                    if (strcmp(node->data.call.func, "get") == 0) {
                        if (url_node->type == NODE_STR) {
                            int url_idx = cg->string_counter++;
                            size_t url_len = actual_string_len(url_node->data.str.value) + 1;
                            int url_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                                    url_ptr, url_len, url_len, url_idx);
                            
                            int headers_ptr = 0;  // Will be null or pointer to headers JSON
                            
                            if (has_auth_bearer && auth_idx >= 0 && (size_t)(auth_idx + 1) < node->data.call.args.count) {
                                // Build headers with Bearer auth
                                // Skip the marker string index (it was collected by collect_strings)
                                cg->string_counter++;  // Skip "__auth_bearer__" marker
                                ASTNode *token_node = node->data.call.args.nodes[auth_idx + 1];
                                if (token_node->type == NODE_STR) {
                                    int token_idx = cg->string_counter++;
                                    size_t token_len = actual_string_len(token_node->data.str.value) + 1;
                                    int token_ptr = next_temp(cg);
                                    fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                                            token_ptr, token_len, token_len, token_idx);
                                    
                                    headers_ptr = next_temp(cg);
                                    fprintf(cg->out, "  %%t%d = call i8* @nerd_http_auth_bearer(i8* %%t%d)\n", 
                                            headers_ptr, token_ptr);
                                }
                            } else if (has_auth_basic && auth_idx >= 0 && (size_t)(auth_idx + 2) < node->data.call.args.count) {
                                // Build headers with Basic auth
                                // Skip the marker string index (it was collected by collect_strings)
                                cg->string_counter++;  // Skip "__auth_basic__" marker
                                ASTNode *user_node = node->data.call.args.nodes[auth_idx + 1];
                                ASTNode *pass_node = node->data.call.args.nodes[auth_idx + 2];
                                if (user_node->type == NODE_STR && pass_node->type == NODE_STR) {
                                    int user_idx = cg->string_counter++;
                                    int pass_idx = cg->string_counter++;
                                    size_t user_len = actual_string_len(user_node->data.str.value) + 1;
                                    size_t pass_len = actual_string_len(pass_node->data.str.value) + 1;
                                    
                                    int user_ptr = next_temp(cg);
                                    fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                                            user_ptr, user_len, user_len, user_idx);
                                    int pass_ptr = next_temp(cg);
                                    fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                                            pass_ptr, pass_len, pass_len, pass_idx);
                                    
                                    headers_ptr = next_temp(cg);
                                    fprintf(cg->out, "  %%t%d = call i8* @nerd_http_auth_basic(i8* %%t%d, i8* %%t%d)\n", 
                                            headers_ptr, user_ptr, pass_ptr);
                                }
                            } else if (header_start >= 0) {
                                // Build headers JSON from with pairs
                                headers_ptr = next_temp(cg);
                                fprintf(cg->out, "  %%t%d = call i8* @nerd_json_new()\n", headers_ptr);
                                
                                // Process header pairs
                                for (size_t i = header_start; i + 1 < node->data.call.args.count; i += 2) {
                                    ASTNode *hname = node->data.call.args.nodes[i];
                                    ASTNode *hvalue = node->data.call.args.nodes[i + 1];
                                    
                                    // Skip auth markers
                                    if (hname->type == NODE_STR && 
                                        (strcmp(hname->data.str.value, "__auth_bearer__") == 0 ||
                                         strcmp(hname->data.str.value, "__auth_basic__") == 0)) {
                                        break;
                                    }
                                    
                                    if (hname->type == NODE_STR && hvalue->type == NODE_STR) {
                                        int hname_idx = cg->string_counter++;
                                        int hvalue_idx = cg->string_counter++;
                                        size_t hname_len = actual_string_len(hname->data.str.value) + 1;
                                        size_t hvalue_len = actual_string_len(hvalue->data.str.value) + 1;
                                        
                                        int hname_ptr = next_temp(cg);
                                        fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                                                hname_ptr, hname_len, hname_len, hname_idx);
                                        int hvalue_ptr = next_temp(cg);
                                        fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                                                hvalue_ptr, hvalue_len, hvalue_len, hvalue_idx);
                                        
                                        fprintf(cg->out, "  call void @nerd_json_set_string(i8* %%t%d, i8* %%t%d, i8* %%t%d)\n",
                                                headers_ptr, hname_ptr, hvalue_ptr);
                                    }
                                }
                            }
                            
                            // Call http_get_full with headers (or null)
                            int response_ptr = next_temp(cg);
                            if (headers_ptr > 0) {
                                fprintf(cg->out, "  %%t%d = call i8* @nerd_http_get_full(i8* %%t%d, i8* %%t%d)\n", 
                                        response_ptr, url_ptr, headers_ptr);
                            } else {
                                fprintf(cg->out, "  %%t%d = call i8* @nerd_http_get_full(i8* %%t%d, i8* null)\n", 
                                        response_ptr, url_ptr);
                            }
                            
                            // Print response via JSON stringify
                            int str_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = call i8* @nerd_json_stringify(i8* %%t%d)\n", str_ptr, response_ptr);
                            fprintf(cg->out, "  call i32 (i8*, ...) @printf(i8* getelementptr ([4 x i8], [4 x i8]* @.fmt_str, i32 0, i32 0), i8* %%t%d)\n", str_ptr);
                            fprintf(cg->out, "  call void @nerd_json_free_string(i8* %%t%d)\n", str_ptr);
                            fprintf(cg->out, "  call void @nerd_json_free(i8* %%t%d)\n", response_ptr);
                            
                            if (headers_ptr > 0) {
                                fprintf(cg->out, "  call void @nerd_json_free(i8* %%t%d)\n", headers_ptr);
                            }
                        }

                        fprintf(cg->out, "  %%t%d = fadd double 0.0, 0.0\n", result_reg);
                        return result_reg;
                    }

                    // HTTP POST (with body and optional headers/auth)
                    if (strcmp(node->data.call.func, "post") == 0 && node->data.call.args.count >= 2) {
                        ASTNode *body_node = node->data.call.args.nodes[1];

                        if (url_node->type == NODE_STR && body_node->type == NODE_STR) {
                            int url_idx = cg->string_counter++;
                            int body_idx = cg->string_counter++;

                            size_t url_len = actual_string_len(url_node->data.str.value) + 1;
                            size_t body_len = actual_string_len(body_node->data.str.value) + 1;

                            int url_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                                    url_ptr, url_len, url_len, url_idx);

                            int body_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                                    body_ptr, body_len, body_len, body_idx);

                            // Check for headers/auth (body_offset=2 for POST)
                            int headers_ptr = 0;
                            
                            if (has_auth_bearer && auth_idx >= 0 && (size_t)(auth_idx + 1) < node->data.call.args.count) {
                                cg->string_counter++;  // Skip marker
                                ASTNode *token_node = node->data.call.args.nodes[auth_idx + 1];
                                if (token_node->type == NODE_STR) {
                                    int token_idx = cg->string_counter++;
                                    size_t token_len = actual_string_len(token_node->data.str.value) + 1;
                                    int token_ptr = next_temp(cg);
                                    fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                                            token_ptr, token_len, token_len, token_idx);
                                    headers_ptr = next_temp(cg);
                                    fprintf(cg->out, "  %%t%d = call i8* @nerd_http_auth_bearer(i8* %%t%d)\n",
                                            headers_ptr, token_ptr);
                                }
                            } else if (header_start >= 0) {
                                headers_ptr = next_temp(cg);
                                fprintf(cg->out, "  %%t%d = call i8* @nerd_json_new()\n", headers_ptr);
                                for (size_t i = header_start; i + 1 < node->data.call.args.count; i += 2) {
                                    ASTNode *hname = node->data.call.args.nodes[i];
                                    ASTNode *hvalue = node->data.call.args.nodes[i + 1];
                                    if (hname->type == NODE_STR && 
                                        (strcmp(hname->data.str.value, "__auth_bearer__") == 0 ||
                                         strcmp(hname->data.str.value, "__auth_basic__") == 0)) break;
                                    if (hname->type == NODE_STR && hvalue->type == NODE_STR) {
                                        int hidx1 = cg->string_counter++;
                                        int hidx2 = cg->string_counter++;
                                        size_t hlen1 = actual_string_len(hname->data.str.value) + 1;
                                        size_t hlen2 = actual_string_len(hvalue->data.str.value) + 1;
                                        int hp1 = next_temp(cg);
                                        int hp2 = next_temp(cg);
                                        fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                                                hp1, hlen1, hlen1, hidx1);
                                        fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                                                hp2, hlen2, hlen2, hidx2);
                                        fprintf(cg->out, "  call void @nerd_json_set_string(i8* %%t%d, i8* %%t%d, i8* %%t%d)\n",
                                                headers_ptr, hp1, hp2);
                                    }
                                }
                            }

                            // Call http_post_full with headers
                            int response_ptr = next_temp(cg);
                            if (headers_ptr > 0) {
                                fprintf(cg->out, "  %%t%d = call i8* @nerd_http_post_full(i8* %%t%d, i8* %%t%d, i8* %%t%d)\n",
                                        response_ptr, url_ptr, body_ptr, headers_ptr);
                            } else {
                                fprintf(cg->out, "  %%t%d = call i8* @nerd_http_post_full(i8* %%t%d, i8* %%t%d, i8* null)\n",
                                        response_ptr, url_ptr, body_ptr);
                            }

                            // Print response via JSON stringify
                            int str_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = call i8* @nerd_json_stringify(i8* %%t%d)\n", str_ptr, response_ptr);
                            fprintf(cg->out, "  call i32 (i8*, ...) @printf(i8* getelementptr ([4 x i8], [4 x i8]* @.fmt_str, i32 0, i32 0), i8* %%t%d)\n", str_ptr);
                            fprintf(cg->out, "  call void @nerd_json_free_string(i8* %%t%d)\n", str_ptr);
                            fprintf(cg->out, "  call void @nerd_json_free(i8* %%t%d)\n", response_ptr);
                            if (headers_ptr > 0) {
                                fprintf(cg->out, "  call void @nerd_json_free(i8* %%t%d)\n", headers_ptr);
                            }
                        }

                        fprintf(cg->out, "  %%t%d = fadd double 0.0, 0.0\n", result_reg);
                        return result_reg;
                    }

                    // HTTP PUT (with body)
                    if (strcmp(node->data.call.func, "put") == 0 && node->data.call.args.count >= 2) {
                        ASTNode *body_node = node->data.call.args.nodes[1];

                        if (url_node->type == NODE_STR && body_node->type == NODE_STR) {
                            int url_idx = cg->string_counter++;
                            int body_idx = cg->string_counter++;

                            size_t url_len = actual_string_len(url_node->data.str.value) + 1;
                            size_t body_len = actual_string_len(body_node->data.str.value) + 1;

                            int url_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                                    url_ptr, url_len, url_len, url_idx);

                            int body_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                                    body_ptr, body_len, body_len, body_idx);

                            // Call http_put (null headers for now)
                            int response_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = call i8* @nerd_http_put(i8* %%t%d, i8* %%t%d, i8* null)\n",
                                    response_ptr, url_ptr, body_ptr);

                            // Print response via JSON stringify
                            int str_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = call i8* @nerd_json_stringify(i8* %%t%d)\n", str_ptr, response_ptr);
                            fprintf(cg->out, "  call i32 (i8*, ...) @printf(i8* getelementptr ([4 x i8], [4 x i8]* @.fmt_str, i32 0, i32 0), i8* %%t%d)\n", str_ptr);
                            fprintf(cg->out, "  call void @nerd_json_free_string(i8* %%t%d)\n", str_ptr);
                            fprintf(cg->out, "  call void @nerd_json_free(i8* %%t%d)\n", response_ptr);
                        }

                        fprintf(cg->out, "  %%t%d = fadd double 0.0, 0.0\n", result_reg);
                        return result_reg;
                    }

                    // HTTP DELETE (no body)
                    if (strcmp(node->data.call.func, "delete") == 0) {
                        if (url_node->type == NODE_STR) {
                            int url_idx = cg->string_counter++;
                            size_t url_len = actual_string_len(url_node->data.str.value) + 1;

                            int url_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                                    url_ptr, url_len, url_len, url_idx);

                            // Call http_delete (null headers for now)
                            int response_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = call i8* @nerd_http_delete(i8* %%t%d, i8* null)\n",
                                    response_ptr, url_ptr);

                            // Print response via JSON stringify
                            int str_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = call i8* @nerd_json_stringify(i8* %%t%d)\n", str_ptr, response_ptr);
                            fprintf(cg->out, "  call i32 (i8*, ...) @printf(i8* getelementptr ([4 x i8], [4 x i8]* @.fmt_str, i32 0, i32 0), i8* %%t%d)\n", str_ptr);
                            fprintf(cg->out, "  call void @nerd_json_free_string(i8* %%t%d)\n", str_ptr);
                            fprintf(cg->out, "  call void @nerd_json_free(i8* %%t%d)\n", response_ptr);
                        }

                        fprintf(cg->out, "  %%t%d = fadd double 0.0, 0.0\n", result_reg);
                        return result_reg;
                    }

                    // HTTP PATCH (with body)
                    if (strcmp(node->data.call.func, "patch") == 0 && node->data.call.args.count >= 2) {
                        ASTNode *body_node = node->data.call.args.nodes[1];

                        if (url_node->type == NODE_STR && body_node->type == NODE_STR) {
                            int url_idx = cg->string_counter++;
                            int body_idx = cg->string_counter++;

                            size_t url_len = actual_string_len(url_node->data.str.value) + 1;
                            size_t body_len = actual_string_len(body_node->data.str.value) + 1;

                            int url_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                                    url_ptr, url_len, url_len, url_idx);

                            int body_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                                    body_ptr, body_len, body_len, body_idx);

                            // Call http_patch (null headers for now)
                            int response_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = call i8* @nerd_http_patch(i8* %%t%d, i8* %%t%d, i8* null)\n",
                                    response_ptr, url_ptr, body_ptr);

                            // Print response via JSON stringify
                            int str_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = call i8* @nerd_json_stringify(i8* %%t%d)\n", str_ptr, response_ptr);
                            fprintf(cg->out, "  call i32 (i8*, ...) @printf(i8* getelementptr ([4 x i8], [4 x i8]* @.fmt_str, i32 0, i32 0), i8* %%t%d)\n", str_ptr);
                            fprintf(cg->out, "  call void @nerd_json_free_string(i8* %%t%d)\n", str_ptr);
                            fprintf(cg->out, "  call void @nerd_json_free(i8* %%t%d)\n", response_ptr);
                        }

                        fprintf(cg->out, "  %%t%d = fadd double 0.0, 0.0\n", result_reg);
                        return result_reg;
                    }
                }
            }

            // MCP module calls
            if (strcmp(node->data.call.module, "mcp") == 0) {
                if (node->data.call.args.count >= 1) {
                    ASTNode *url_node = node->data.call.args.nodes[0];

                    // mcp tools url - list tools from MCP server
                    if (strcmp(node->data.call.func, "tools") == 0) {
                        if (url_node->type == NODE_STR) {
                            int url_idx = cg->string_counter++;
                            size_t url_len = actual_string_len(url_node->data.str.value) + 1;

                            int url_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                                    url_ptr, url_len, url_len, url_idx);

                            // Call mcp_list
                            int response_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = call i8* @nerd_mcp_list(i8* %%t%d)\n", response_ptr, url_ptr);

                            // Free response
                            fprintf(cg->out, "  call void @nerd_mcp_free(i8* %%t%d)\n", response_ptr);
                        }

                        fprintf(cg->out, "  %%t%d = fadd double 0.0, 0.0\n", result_reg);
                        return result_reg;
                    }

                    // mcp send url tool_name args_json - call a tool
                    if (strcmp(node->data.call.func, "send") == 0 && node->data.call.args.count >= 3) {
                        ASTNode *tool_node = node->data.call.args.nodes[1];
                        ASTNode *args_node = node->data.call.args.nodes[2];

                        if (url_node->type == NODE_STR && tool_node->type == NODE_STR && args_node->type == NODE_STR) {
                            int url_idx = cg->string_counter++;
                            int tool_idx = cg->string_counter++;
                            int args_idx = cg->string_counter++;

                            size_t url_len = actual_string_len(url_node->data.str.value) + 1;
                            size_t tool_len = actual_string_len(tool_node->data.str.value) + 1;
                            size_t args_len = actual_string_len(args_node->data.str.value) + 1;

                            int url_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                                    url_ptr, url_len, url_len, url_idx);

                            int tool_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                                    tool_ptr, tool_len, tool_len, tool_idx);

                            int args_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                                    args_ptr, args_len, args_len, args_idx);

                            // Call mcp_send
                            int response_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = call i8* @nerd_mcp_send(i8* %%t%d, i8* %%t%d, i8* %%t%d)\n",
                                    response_ptr, url_ptr, tool_ptr, args_ptr);

                            // Free response
                            fprintf(cg->out, "  call void @nerd_mcp_free(i8* %%t%d)\n", response_ptr);
                        }

                        fprintf(cg->out, "  %%t%d = fadd double 0.0, 0.0\n", result_reg);
                        return result_reg;
                    }

                    // mcp init url - initialize MCP session (optional)
                    if (strcmp(node->data.call.func, "init") == 0) {
                        if (url_node->type == NODE_STR) {
                            int url_idx = cg->string_counter++;
                            size_t url_len = actual_string_len(url_node->data.str.value) + 1;

                            int url_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                                    url_ptr, url_len, url_len, url_idx);

                            // Call mcp_init
                            int response_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = call i8* @nerd_mcp_init(i8* %%t%d)\n", response_ptr, url_ptr);

                            // Free response
                            fprintf(cg->out, "  call void @nerd_mcp_free(i8* %%t%d)\n", response_ptr);
                        }

                        fprintf(cg->out, "  %%t%d = fadd double 0.0, 0.0\n", result_reg);
                        return result_reg;
                    }
                }

                // Default for mcp module
                fprintf(cg->out, "  %%t%d = fadd double 0.0, 0.0\n", result_reg);
                return result_reg;
            }

            // LLM module calls
            if (strcmp(node->data.call.module, "llm") == 0) {
                if (node->data.call.args.count >= 1) {
                    ASTNode *prompt_node = node->data.call.args.nodes[0];

                    // llm claude "prompt" - Call Claude
                    if (strcmp(node->data.call.func, "claude") == 0) {
                        if (prompt_node->type == NODE_STR) {
                            int prompt_idx = cg->string_counter++;
                            size_t prompt_len = actual_string_len(prompt_node->data.str.value) + 1;

                            int prompt_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                                    prompt_ptr, prompt_len, prompt_len, prompt_idx);

                            // Call llm_claude
                            int response_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = call i8* @nerd_llm_claude(i8* %%t%d)\n", response_ptr, prompt_ptr);

                            // Free response
                            fprintf(cg->out, "  call void @nerd_llm_free(i8* %%t%d)\n", response_ptr);
                        }

                        fprintf(cg->out, "  %%t%d = fadd double 0.0, 0.0\n", result_reg);
                        return result_reg;
                    }
                }

                fprintf(cg->out, "  %%t%d = fadd double 0.0, 0.0\n", result_reg);
                return result_reg;
            }

            // Default: return 0 for unimplemented calls
            fprintf(cg->out, "  %%t%d = fadd double 0.0, 0.0\n", result_reg);
            return result_reg;
        }

        case NODE_JSON_NEW: {
            // Create new empty JSON object
            int result_reg = next_temp(cg);
            fprintf(cg->out, "  %%t%d = call i8* @nerd_json_new()\n", result_reg);
            return result_reg;
        }

        case NODE_JSON_ACCESS: {
            // obj."path" - get value from JSON
            int obj_reg;
            
            // Check if object is a pointer local (JSON variable)
            if (node->data.json_access.object->type == NODE_VAR) {
                const char *var_name = node->data.json_access.object->data.var.name;
                int ptr_local = find_ptr_local(cg, var_name);
                if (ptr_local >= 0) {
                    // Load pointer from pointer local
                    obj_reg = next_temp(cg);
                    fprintf(cg->out, "  %%t%d = load i8*, i8** %%plocal%d\n", obj_reg, ptr_local);
                } else {
                    // Fall back to normal expression evaluation
                    obj_reg = codegen_expr(cg, node->data.json_access.object);
                }
            } else {
                obj_reg = codegen_expr(cg, node->data.json_access.object);
            }
            if (obj_reg < 0) return -1;

            // Use pre-collected string index
            int path_idx = cg->string_counter++;
            size_t path_len = strlen(node->data.json_access.path) + 1;
            int path_ptr = next_temp(cg);
            fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                    path_ptr, path_len, path_len, path_idx);

            // Call get_number by default (we'll need type inference later)
            int result_reg = next_temp(cg);
            fprintf(cg->out, "  %%t%d = call double @nerd_json_get_number(i8* %%t%d, i8* %%t%d)\n",
                    result_reg, obj_reg, path_ptr);
            return result_reg;
        }

        case NODE_JSON_HAS: {
            // obj?"key" - check if key exists
            int obj_reg = codegen_expr(cg, node->data.json_has.object);
            if (obj_reg < 0) return -1;

            int path_idx = cg->string_counter++;
            size_t path_len = strlen(node->data.json_has.path) + 1;
            int path_ptr = next_temp(cg);
            fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                    path_ptr, path_len, path_len, path_idx);

            int has_reg = next_temp(cg);
            fprintf(cg->out, "  %%t%d = call i32 @nerd_json_has(i8* %%t%d, i8* %%t%d)\n",
                    has_reg, obj_reg, path_ptr);

            // Convert to double for NERD's type system
            int result_reg = next_temp(cg);
            fprintf(cg->out, "  %%t%d = sitofp i32 %%t%d to double\n", result_reg, has_reg);
            return result_reg;
        }

        case NODE_JSON_COUNT: {
            // obj."path".count - get array length
            int obj_reg = codegen_expr(cg, node->data.json_count.object);
            if (obj_reg < 0) return -1;

            int path_ptr;
            if (node->data.json_count.path) {
                int path_idx = cg->string_counter++;
                size_t path_len = strlen(node->data.json_count.path) + 1;
                path_ptr = next_temp(cg);
                fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                        path_ptr, path_len, path_len, path_idx);
            } else {
                // NULL path for root level
                path_ptr = next_temp(cg);
                fprintf(cg->out, "  %%t%d = inttoptr i64 0 to i8*\n", path_ptr);
            }

            int count_reg = next_temp(cg);
            fprintf(cg->out, "  %%t%d = call i32 @nerd_json_count(i8* %%t%d, i8* %%t%d)\n",
                    count_reg, obj_reg, path_ptr);

            // Convert to double
            int result_reg = next_temp(cg);
            fprintf(cg->out, "  %%t%d = sitofp i32 %%t%d to double\n", result_reg, count_reg);
            return result_reg;
        }

        default:
            fprintf(stderr, "Error: Unknown expression node type %d\n", node->type);
            return -1;
    }
}

/*
 * Generate code for statement
 */
static void codegen_stmt(CodeGen *cg, ASTNode *node, int *result_reg) {
    if (!node) return;

    switch (node->type) {
        case NODE_RETURN: {
            int val_reg = codegen_expr(cg, node->data.ret.value);
            if (val_reg >= 0) {
                fprintf(cg->out, "  ret double %%t%d\n", val_reg);
            }
            break;
        }

        case NODE_IF: {
            int cond_reg = codegen_expr(cg, node->data.if_stmt.condition);
            if (cond_reg < 0) return;

            int bool_reg = next_temp(cg);
            int then_label = next_label(cg);
            int else_label = next_label(cg);
            int end_label = next_label(cg);

            fprintf(cg->out, "  %%t%d = fcmp one double %%t%d, 0.0\n", bool_reg, cond_reg);

            if (node->data.if_stmt.else_stmt) {
                // Has else branch
                fprintf(cg->out, "  br i1 %%t%d, label %%then%d, label %%else%d\n", bool_reg, then_label, else_label);

                // Then block
                fprintf(cg->out, "then%d:\n", then_label);
                codegen_stmt(cg, node->data.if_stmt.then_stmt, result_reg);
                bool then_returns = (node->data.if_stmt.then_stmt->type == NODE_RETURN);
                if (!then_returns) {
                    fprintf(cg->out, "  br label %%end%d\n", end_label);
                }

                // Else block
                fprintf(cg->out, "else%d:\n", else_label);
                codegen_stmt(cg, node->data.if_stmt.else_stmt, result_reg);

                // Check if else needs a branch to end
                bool else_returns = (node->data.if_stmt.else_stmt->type == NODE_RETURN);

                if (!else_returns) {
                    // Always branch to end after else block (including after nested if)
                    fprintf(cg->out, "  br label %%end%d\n", end_label);
                }

                // Always emit end label for if-else (needed for merging control flow)
                fprintf(cg->out, "end%d:\n", end_label);
            } else {
                // No else branch
                fprintf(cg->out, "  br i1 %%t%d, label %%then%d, label %%end%d\n", bool_reg, then_label, end_label);

                fprintf(cg->out, "then%d:\n", then_label);
                codegen_stmt(cg, node->data.if_stmt.then_stmt, result_reg);

                if (node->data.if_stmt.then_stmt->type != NODE_RETURN) {
                    fprintf(cg->out, "  br label %%end%d\n", end_label);
                }

                fprintf(cg->out, "end%d:\n", end_label);
            }
            break;
        }

        case NODE_LET: {
            // Check if this is a JSON new (let x {})
            if (node->data.let.value->type == NODE_JSON_NEW) {
                int json_reg = next_temp(cg);
                fprintf(cg->out, "  %%t%d = call i8* @nerd_json_new()\n", json_reg);
                
                // Store as pointer local
                int ptr_local_id = (int)cg->ptr_local_count;
                fprintf(cg->out, "  %%plocal%d = alloca i8*\n", ptr_local_id);
                fprintf(cg->out, "  store i8* %%t%d, i8** %%plocal%d\n", json_reg, ptr_local_id);
                add_ptr_local(cg, node->data.let.name, ptr_local_id);
                break;
            }

            // Check if this is an HTTP call (let x http get "url")
            ASTNode *val = node->data.let.value;
            if (val->type == NODE_CALL && val->data.call.module &&
                strcmp(val->data.call.module, "http") == 0 &&
                val->data.call.args.count >= 1) {
                
                ASTNode *url_node = val->data.call.args.nodes[0];
                
                if (strcmp(val->data.call.func, "get") == 0 && url_node->type == NODE_STR) {
                    // let x http get "url" -> store JSON response
                    int url_idx = cg->string_counter++;
                    if (cg->string_count >= cg->string_capacity) {
                        cg->string_capacity *= 2;
                        cg->string_literals = realloc(cg->string_literals,
                            sizeof(char*) * cg->string_capacity);
                    }
                    cg->string_literals[cg->string_count++] = nerd_strdup(url_node->data.str.value);

                    size_t url_len = actual_string_len(url_node->data.str.value) + 1;
                    int url_ptr = next_temp(cg);
                    fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                            url_ptr, url_len, url_len, url_idx);

                    // Call http_get_json
                    int json_reg = next_temp(cg);
                    fprintf(cg->out, "  %%t%d = call i8* @nerd_http_get_json(i8* %%t%d)\n", json_reg, url_ptr);

                    // Store as pointer local
                    int ptr_local_id = (int)cg->ptr_local_count;
                    fprintf(cg->out, "  %%plocal%d = alloca i8*\n", ptr_local_id);
                    fprintf(cg->out, "  store i8* %%t%d, i8** %%plocal%d\n", json_reg, ptr_local_id);
                    add_ptr_local(cg, node->data.let.name, ptr_local_id);
                    break;
                }

                if (strcmp(val->data.call.func, "post") == 0 && 
                    val->data.call.args.count >= 2 &&
                    url_node->type == NODE_STR) {
                    // let x http post "url" body
                    ASTNode *body_node = val->data.call.args.nodes[1];
                    
                    int url_idx = cg->string_counter++;
                    if (cg->string_count >= cg->string_capacity) {
                        cg->string_capacity *= 2;
                        cg->string_literals = realloc(cg->string_literals,
                            sizeof(char*) * cg->string_capacity);
                    }
                    cg->string_literals[cg->string_count++] = nerd_strdup(url_node->data.str.value);

                    size_t url_len = actual_string_len(url_node->data.str.value) + 1;
                    int url_ptr = next_temp(cg);
                    fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                            url_ptr, url_len, url_len, url_idx);

                    int json_reg;
                    if (body_node->type == NODE_STR) {
                        // String body
                        int body_idx = cg->string_counter++;
                        if (cg->string_count >= cg->string_capacity) {
                            cg->string_capacity *= 2;
                            cg->string_literals = realloc(cg->string_literals,
                                sizeof(char*) * cg->string_capacity);
                        }
                        cg->string_literals[cg->string_count++] = nerd_strdup(body_node->data.str.value);

                        size_t body_len = actual_string_len(body_node->data.str.value) + 1;
                        int body_ptr = next_temp(cg);
                        fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                                body_ptr, body_len, body_len, body_idx);

                        json_reg = next_temp(cg);
                        fprintf(cg->out, "  %%t%d = call i8* @nerd_http_post_json(i8* %%t%d, i8* %%t%d)\n",
                                json_reg, url_ptr, body_ptr);
                    } else if (body_node->type == NODE_VAR) {
                        // Variable body (check if it's a JSON object)
                        int ptr_local = find_ptr_local(cg, body_node->data.var.name);
                        if (ptr_local >= 0) {
                            int body_ptr = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = load i8*, i8** %%plocal%d\n", body_ptr, ptr_local);
                            json_reg = next_temp(cg);
                            fprintf(cg->out, "  %%t%d = call i8* @nerd_http_post_json_body(i8* %%t%d, i8* %%t%d)\n",
                                    json_reg, url_ptr, body_ptr);
                        } else {
                            fprintf(stderr, "Error: HTTP POST body must be a string or JSON object\n");
                            return;
                        }
                    } else {
                        fprintf(stderr, "Error: HTTP POST body must be a string or JSON object\n");
                        return;
                    }

                    // Store as pointer local
                    int ptr_local_id = (int)cg->ptr_local_count;
                    fprintf(cg->out, "  %%plocal%d = alloca i8*\n", ptr_local_id);
                    fprintf(cg->out, "  store i8* %%t%d, i8** %%plocal%d\n", json_reg, ptr_local_id);
                    add_ptr_local(cg, node->data.let.name, ptr_local_id);
                    break;
                }
            }
            
            int val_reg = codegen_expr(cg, node->data.let.value);
            if (val_reg < 0) return;

            // Check if variable already exists (reassignment)
            int existing = find_local(cg, node->data.let.name);
            if (existing >= 0) {
                // Update existing variable
                fprintf(cg->out, "  store double %%t%d, double* %%local%d\n", val_reg, existing);
            } else {
                // Create new variable
                int local_id = (int)cg->local_count;
                fprintf(cg->out, "  %%local%d = alloca double\n", local_id);
                fprintf(cg->out, "  store double %%t%d, double* %%local%d\n", val_reg, local_id);
                add_local(cg, node->data.let.name, local_id);
            }
            break;
        }

        case NODE_EXPR_STMT: {
            codegen_expr(cg, node->data.expr_stmt.expr);
            break;
        }

        case NODE_REPEAT: {
            // repeat n times [as i] ... done
            // Generates: for (i = 1; i <= n; i++) { body }

            int count_reg = codegen_expr(cg, node->data.repeat.count);
            if (count_reg < 0) return;

            int loop_start = next_label(cg);
            int loop_body = next_label(cg);
            int loop_end = next_label(cg);

            // Allocate counter variable (starts at 1)
            int counter_id = (int)cg->local_count;
            fprintf(cg->out, "  %%local%d = alloca double\n", counter_id);
            fprintf(cg->out, "  store double 1.0, double* %%local%d\n", counter_id);

            // If there's an 'as' variable, set up the binding
            if (node->data.repeat.var_name) {
                add_local(cg, node->data.repeat.var_name, counter_id);
            } else {
                // Need to track the counter even without a name
                cg->local_count++;
            }

            // Loop condition check
            fprintf(cg->out, "  br label %%loop_start%d\n", loop_start);
            fprintf(cg->out, "loop_start%d:\n", loop_start);

            int counter_val = next_temp(cg);
            fprintf(cg->out, "  %%t%d = load double, double* %%local%d\n", counter_val, counter_id);

            int cmp_reg = next_temp(cg);
            fprintf(cg->out, "  %%t%d = fcmp ole double %%t%d, %%t%d\n", cmp_reg, counter_val, count_reg);
            fprintf(cg->out, "  br i1 %%t%d, label %%loop_body%d, label %%loop_end%d\n", cmp_reg, loop_body, loop_end);

            // Loop body
            fprintf(cg->out, "loop_body%d:\n", loop_body);
            for (size_t i = 0; i < node->data.repeat.body.count; i++) {
                codegen_stmt(cg, node->data.repeat.body.nodes[i], result_reg);
            }

            // Increment counter
            int inc_load = next_temp(cg);
            int inc_add = next_temp(cg);
            fprintf(cg->out, "  %%t%d = load double, double* %%local%d\n", inc_load, counter_id);
            fprintf(cg->out, "  %%t%d = fadd double %%t%d, 1.0\n", inc_add, inc_load);
            fprintf(cg->out, "  store double %%t%d, double* %%local%d\n", inc_add, counter_id);
            fprintf(cg->out, "  br label %%loop_start%d\n", loop_start);

            // Loop end
            fprintf(cg->out, "loop_end%d:\n", loop_end);
            break;
        }

        case NODE_WHILE: {
            // while cond ... done

            int loop_start = next_label(cg);
            int loop_body = next_label(cg);
            int loop_end = next_label(cg);

            // Loop condition check
            fprintf(cg->out, "  br label %%while_start%d\n", loop_start);
            fprintf(cg->out, "while_start%d:\n", loop_start);

            int cond_reg = codegen_expr(cg, node->data.while_loop.condition);
            if (cond_reg < 0) return;

            int bool_reg = next_temp(cg);
            fprintf(cg->out, "  %%t%d = fcmp one double %%t%d, 0.0\n", bool_reg, cond_reg);
            fprintf(cg->out, "  br i1 %%t%d, label %%while_body%d, label %%while_end%d\n", bool_reg, loop_body, loop_end);

            // Loop body
            fprintf(cg->out, "while_body%d:\n", loop_body);
            for (size_t i = 0; i < node->data.while_loop.body.count; i++) {
                codegen_stmt(cg, node->data.while_loop.body.nodes[i], result_reg);
            }
            fprintf(cg->out, "  br label %%while_start%d\n", loop_start);

            // Loop end
            fprintf(cg->out, "while_end%d:\n", loop_end);
            break;
        }

        case NODE_INC: {
            // inc var [amount] - increment variable
            int existing = find_local(cg, node->data.inc.var_name);
            if (existing < 0) {
                fprintf(stderr, "Error: Unknown variable '%s' in inc\n", node->data.inc.var_name);
                return;
            }

            int load_reg = next_temp(cg);
            fprintf(cg->out, "  %%t%d = load double, double* %%local%d\n", load_reg, existing);

            int amount_reg;
            if (node->data.inc.amount) {
                amount_reg = codegen_expr(cg, node->data.inc.amount);
            } else {
                amount_reg = next_temp(cg);
                fprintf(cg->out, "  %%t%d = fadd double 0.0, 1.0\n", amount_reg);
            }

            int result_reg = next_temp(cg);
            fprintf(cg->out, "  %%t%d = fadd double %%t%d, %%t%d\n", result_reg, load_reg, amount_reg);
            fprintf(cg->out, "  store double %%t%d, double* %%local%d\n", result_reg, existing);
            break;
        }

        case NODE_DEC: {
            // dec var [amount] - decrement variable
            int existing = find_local(cg, node->data.dec.var_name);
            if (existing < 0) {
                fprintf(stderr, "Error: Unknown variable '%s' in dec\n", node->data.dec.var_name);
                return;
            }

            int load_reg = next_temp(cg);
            fprintf(cg->out, "  %%t%d = load double, double* %%local%d\n", load_reg, existing);

            int amount_reg;
            if (node->data.dec.amount) {
                amount_reg = codegen_expr(cg, node->data.dec.amount);
            } else {
                amount_reg = next_temp(cg);
                fprintf(cg->out, "  %%t%d = fadd double 0.0, 1.0\n", amount_reg);
            }

            int result_reg = next_temp(cg);
            fprintf(cg->out, "  %%t%d = fsub double %%t%d, %%t%d\n", result_reg, load_reg, amount_reg);
            fprintf(cg->out, "  store double %%t%d, double* %%local%d\n", result_reg, existing);
            break;
        }

        case NODE_JSON_SET: {
            // x."key" = value - set value in JSON object
            
            // Get the JSON object pointer
            int obj_reg;
            if (node->data.json_set.object->type == NODE_VAR) {
                const char *var_name = node->data.json_set.object->data.var.name;
                int ptr_local = find_ptr_local(cg, var_name);
                if (ptr_local >= 0) {
                    obj_reg = next_temp(cg);
                    fprintf(cg->out, "  %%t%d = load i8*, i8** %%plocal%d\n", obj_reg, ptr_local);
                } else {
                    fprintf(stderr, "Error: '%s' is not a JSON object\n", var_name);
                    return;
                }
            } else {
                fprintf(stderr, "Error: JSON set requires a variable\n");
                return;
            }

            // Get key string (pre-collected)
            int key_idx = cg->string_counter++;
            size_t key_len = strlen(node->data.json_set.key) + 1;
            int key_ptr = next_temp(cg);
            fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                    key_ptr, key_len, key_len, key_idx);

            // Get value
            ASTNode *val = node->data.json_set.value;
            if (val->type == NODE_STR) {
                // String value (pre-collected)
                int val_idx = cg->string_counter++;
                size_t val_len = actual_string_len(val->data.str.value) + 1;
                int val_ptr = next_temp(cg);
                fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                        val_ptr, val_len, val_len, val_idx);

                fprintf(cg->out, "  call void @nerd_json_set_string(i8* %%t%d, i8* %%t%d, i8* %%t%d)\n",
                        obj_reg, key_ptr, val_ptr);
            } else if (val->type == NODE_BOOL) {
                // Boolean value
                int bool_val = val->data.boolean.value ? 1 : 0;
                fprintf(cg->out, "  call void @nerd_json_set_bool(i8* %%t%d, i8* %%t%d, i32 %d)\n",
                        obj_reg, key_ptr, bool_val);
            } else {
                // Numeric value
                int val_reg = codegen_expr(cg, val);
                if (val_reg >= 0) {
                    fprintf(cg->out, "  call void @nerd_json_set_number(i8* %%t%d, i8* %%t%d, double %%t%d)\n",
                            obj_reg, key_ptr, val_reg);
                }
            }
            break;
        }

        case NODE_OUT: {
            ASTNode *val = node->data.out.value;

            if (val->type == NODE_STR) {
                // Output string literal - find its index in pre-collected strings
                int str_id = cg->string_counter++;
                size_t len = actual_string_len(val->data.str.value);

                // Get string pointer and call printf
                int ptr_reg = next_temp(cg);
                fprintf(cg->out, "  %%t%d = getelementptr [%zu x i8], [%zu x i8]* @.str%d, i32 0, i32 0\n",
                        ptr_reg, len + 1, len + 1, str_id);
                fprintf(cg->out, "  call i32 (i8*, ...) @printf(i8* getelementptr ([4 x i8], [4 x i8]* @.fmt_str, i32 0, i32 0), i8* %%t%d)\n",
                        ptr_reg);
            } else {
                // Output number
                int val_reg = codegen_expr(cg, val);
                if (val_reg >= 0) {
                    fprintf(cg->out, "  call i32 (i8*, ...) @printf(i8* getelementptr ([4 x i8], [4 x i8]* @.fmt_num, i32 0, i32 0), double %%t%d)\n",
                            val_reg);
                }
            }
            break;
        }

        default:
            fprintf(stderr, "Error: Unknown statement node type %d\n", node->type);
            break;
    }
}

/*
 * Generate code for function
 */
static void codegen_func(CodeGen *cg, ASTNode *func) {
    clear_locals(cg);
    cg->current_func = func;

    // Set up parameter names
    cg->param_count = func->data.func_def.params.count;
    cg->param_names = malloc(sizeof(char*) * cg->param_count);
    for (size_t i = 0; i < cg->param_count; i++) {
        cg->param_names[i] = func->data.func_def.params.nodes[i]->data.param.name;
    }

    // Function signature
    fprintf(cg->out, "define double @%s(", func->data.func_def.name);
    for (size_t i = 0; i < cg->param_count; i++) {
        if (i > 0) fprintf(cg->out, ", ");
        fprintf(cg->out, "double %%arg%zu", i);
    }
    fprintf(cg->out, ") {\n");
    fprintf(cg->out, "entry:\n");

    // Generate body
    int result_reg = -1;
    bool has_return = false;
    for (size_t i = 0; i < func->data.func_def.body.count; i++) {
        ASTNode *stmt = func->data.func_def.body.nodes[i];
        codegen_stmt(cg, stmt, &result_reg);
        if (stmt->type == NODE_RETURN) {
            has_return = true;
        }
    }

    // Default return if no explicit return (required for valid LLVM IR)
    if (!has_return) {
        fprintf(cg->out, "  ret double 0.0\n");
    }
    fprintf(cg->out, "}\n\n");

    free(cg->param_names);
    cg->param_names = NULL;
    cg->param_count = 0;
}

/*
 * Generate LLVM IR for program
 */
bool codegen_llvm(NerdContext *ctx, const char *output_path) {
    FILE *out = fopen(output_path, "w");
    if (!out) {
        ctx->error_msg = nerd_strdup("Failed to open output file");
        return false;
    }

    CodeGen *cg = codegen_create(out);
    if (!cg) {
        fclose(out);
        ctx->error_msg = nerd_strdup("Failed to create code generator");
        return false;
    }

    // Header
    fprintf(out, "; NERD Compiled Program\n");
    fprintf(out, "; Generated by NERD Bootstrap Compiler\n\n");

    // Declare LLVM intrinsics
    fprintf(out, "declare double @llvm.fabs.f64(double)\n");
    fprintf(out, "declare double @llvm.sqrt.f64(double)\n");
    fprintf(out, "declare double @llvm.floor.f64(double)\n");
    fprintf(out, "declare double @llvm.ceil.f64(double)\n");
    fprintf(out, "declare double @llvm.sin.f64(double)\n");
    fprintf(out, "declare double @llvm.cos.f64(double)\n");
    fprintf(out, "declare double @llvm.pow.f64(double, double)\n");
    fprintf(out, "declare double @llvm.minnum.f64(double, double)\n");
    fprintf(out, "declare double @llvm.maxnum.f64(double, double)\n");
    fprintf(out, "\n");

    // Declare printf for output
    fprintf(out, "declare i32 @printf(i8*, ...)\n");
    fprintf(out, "\n");

    // HTTP runtime declarations (legacy)
    fprintf(out, "declare i8* @nerd_http_get(i8*)\n");
    fprintf(out, "declare i8* @nerd_http_post(i8*, i8*)\n");
    fprintf(out, "declare void @nerd_http_free(i8*)\n");
    fprintf(out, "declare i8* @nerd_http_get_json(i8*)\n");
    fprintf(out, "declare i8* @nerd_http_post_json(i8*, i8*)\n");
    fprintf(out, "declare i8* @nerd_http_post_json_body(i8*, i8*)\n");
    // HTTP runtime declarations (full)
    fprintf(out, "declare i8* @nerd_http_request(i8*, i8*, i8*, i8*)\n");
    fprintf(out, "declare i8* @nerd_http_get_full(i8*, i8*)\n");
    fprintf(out, "declare i8* @nerd_http_post_full(i8*, i8*, i8*)\n");
    fprintf(out, "declare i8* @nerd_http_put(i8*, i8*, i8*)\n");
    fprintf(out, "declare i8* @nerd_http_delete(i8*, i8*)\n");
    fprintf(out, "declare i8* @nerd_http_patch(i8*, i8*, i8*)\n");
    fprintf(out, "declare i8* @nerd_http_auth_bearer(i8*)\n");
    fprintf(out, "declare i8* @nerd_http_auth_basic(i8*, i8*)\n");
    fprintf(out, "\n");

    // MCP runtime declarations
    fprintf(out, "declare i8* @nerd_mcp_list(i8*)\n");
    fprintf(out, "declare i8* @nerd_mcp_send(i8*, i8*, i8*)\n");
    fprintf(out, "declare i8* @nerd_mcp_init(i8*)\n");
    fprintf(out, "declare void @nerd_mcp_free(i8*)\n");
    fprintf(out, "\n");

    // LLM runtime declarations
    fprintf(out, "declare i8* @nerd_llm_claude(i8*)\n");
    fprintf(out, "declare void @nerd_llm_free(i8*)\n");
    fprintf(out, "\n");

    // JSON runtime declarations
    fprintf(out, "declare i8* @nerd_json_new()\n");
    fprintf(out, "declare i8* @nerd_json_parse(i8*)\n");
    fprintf(out, "declare i8* @nerd_json_get_string(i8*, i8*)\n");
    fprintf(out, "declare double @nerd_json_get_number(i8*, i8*)\n");
    fprintf(out, "declare i32 @nerd_json_get_bool(i8*, i8*)\n");
    fprintf(out, "declare i8* @nerd_json_get_object(i8*, i8*)\n");
    fprintf(out, "declare i32 @nerd_json_count(i8*, i8*)\n");
    fprintf(out, "declare i32 @nerd_json_has(i8*, i8*)\n");
    fprintf(out, "declare void @nerd_json_set_string(i8*, i8*, i8*)\n");
    fprintf(out, "declare void @nerd_json_set_number(i8*, i8*, double)\n");
    fprintf(out, "declare void @nerd_json_set_bool(i8*, i8*, i32)\n");
    fprintf(out, "declare i8* @nerd_json_stringify(i8*)\n");
    fprintf(out, "declare void @nerd_json_free(i8*)\n");
    fprintf(out, "declare void @nerd_json_free_string(i8*)\n");
    fprintf(out, "\n");

    // Format strings for output
    fprintf(out, "@.fmt_num = private constant [4 x i8] c\"%%g\\0A\\00\"\n");
    fprintf(out, "@.fmt_str = private constant [4 x i8] c\"%%s\\0A\\00\"\n");
    fprintf(out, "@.fmt_int = private constant [6 x i8] c\"%%.0f\\0A\\00\"\n");
    fprintf(out, "\n");

    // Collect all string literals from AST
    ASTNode *program = ctx->ast;
    collect_strings(cg, program);

    // Output string literal declarations
    for (size_t i = 0; i < cg->string_count; i++) {
        const char *s = cg->string_literals[i];
        size_t src_len = strlen(s);

        // First pass: count actual length after processing escapes
        size_t actual_len = 0;
        for (size_t j = 0; j < src_len; j++) {
            if (s[j] == '\\' && j + 1 < src_len) {
                j++;  // Skip escape sequence, counts as 1 char
            }
            actual_len++;
        }

        fprintf(out, "@.str%zu = private constant [%zu x i8] c\"", i, actual_len + 1);
        for (size_t j = 0; j < src_len; j++) {
            char c = s[j];
            if (c == '\\' && j + 1 < src_len) {
                // Handle escape sequences
                char next = s[j + 1];
                if (next == '"') {
                    fprintf(out, "\\22");  // Quote
                    j++;
                } else if (next == '\\') {
                    fprintf(out, "\\5C");  // Backslash
                    j++;
                } else if (next == 'n') {
                    fprintf(out, "\\0A");  // Newline
                    j++;
                } else if (next == 't') {
                    fprintf(out, "\\09");  // Tab
                    j++;
                } else {
                    // Unknown escape, output as-is
                    fprintf(out, "\\5C");
                }
            } else if (c == '"') {
                fprintf(out, "\\22");
            } else if (c >= 32 && c < 127) {
                fputc(c, out);
            } else {
                fprintf(out, "\\%02X", (unsigned char)c);
            }
        }
        fprintf(out, "\\00\"\n");
    }
    if (cg->string_count > 0) {
        fprintf(out, "\n");
    }

    // Generate functions
    for (size_t i = 0; i < program->data.program.functions.count; i++) {
        codegen_func(cg, program->data.program.functions.nodes[i]);
    }

    codegen_free(cg);
    fclose(out);
    return true;
}
