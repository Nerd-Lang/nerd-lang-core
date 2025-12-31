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

    // Current function context
    ASTNode *current_func;
    char **param_names;
    size_t param_count;

    // Local variables
    char **local_names;
    int *local_regs;
    size_t local_count;
    size_t local_capacity;
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
    cg->local_capacity = 16;
    cg->local_names = malloc(sizeof(char*) * cg->local_capacity);
    cg->local_regs = malloc(sizeof(int) * cg->local_capacity);

    return cg;
}

static void codegen_free(CodeGen *cg) {
    if (!cg) return;
    for (size_t i = 0; i < cg->local_count; i++) {
        free(cg->local_names[i]);
    }
    free(cg->local_names);
    free(cg->local_regs);
    free(cg);
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
    cg->temp_counter = 0;
}

/*
 * Forward declaration
 */
static int codegen_expr(CodeGen *cg, ASTNode *node);
static void codegen_stmt(CodeGen *cg, ASTNode *node, int *result_reg);

/*
 * Generate code for expression, returns register number
 */
static int codegen_expr(CodeGen *cg, ASTNode *node) {
    if (!node) return -1;

    switch (node->type) {
        case NODE_NUM: {
            int reg = next_temp(cg);
            fprintf(cg->out, "  %%t%d = fadd double 0.0, %.17g\n", reg, node->data.num.value);
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

            // Default: return 0 for unimplemented calls
            fprintf(cg->out, "  %%t%d = fadd double 0.0, 0.0\n", result_reg);
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
            int end_label = next_label(cg);

            fprintf(cg->out, "  %%t%d = fcmp one double %%t%d, 0.0\n", bool_reg, cond_reg);
            fprintf(cg->out, "  br i1 %%t%d, label %%then%d, label %%end%d\n", bool_reg, then_label, end_label);

            fprintf(cg->out, "then%d:\n", then_label);
            codegen_stmt(cg, node->data.if_stmt.then_stmt, result_reg);

            // Only add branch if then block didn't return
            if (node->data.if_stmt.then_stmt->type != NODE_RETURN) {
                fprintf(cg->out, "  br label %%end%d\n", end_label);
            }

            fprintf(cg->out, "end%d:\n", end_label);
            break;
        }

        case NODE_LET: {
            int val_reg = codegen_expr(cg, node->data.let.value);
            if (val_reg < 0) return;

            int local_id = (int)cg->local_count;
            fprintf(cg->out, "  %%local%d = alloca double\n", local_id);
            fprintf(cg->out, "  store double %%t%d, double* %%local%d\n", val_reg, local_id);
            add_local(cg, node->data.let.name, local_id);
            break;
        }

        case NODE_EXPR_STMT: {
            codegen_expr(cg, node->data.expr_stmt.expr);
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

    // Generate functions
    ASTNode *program = ctx->ast;
    for (size_t i = 0; i < program->data.program.functions.count; i++) {
        codegen_func(cg, program->data.program.functions.nodes[i]);
    }

    codegen_free(cg);
    fclose(out);
    return true;
}
