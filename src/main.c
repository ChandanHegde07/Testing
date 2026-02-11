/**
 * @file main.c
 * @brief Main entry point for PCC compiler
 */

#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "optimizer.h"
#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Print usage information */
static void print_usage(const char* program_name) {
    printf("PCC Compiler v%s\n", PCC_VERSION);
    printf("Usage: %s [options] <input_file> [output_file]\n", program_name);
    printf("\nOptions:\n");
    printf("  -h, --help        Show this help message\n");
    printf("  -v, --version     Show version information\n");
    printf("  -o <file>         Specify output file\n");
    printf("  -f <format>       Output format: json, text, markdown (default: json)\n");
    printf("  -O                Enable optimizations\n");
    printf("  --no-optimize     Disable optimizations\n");
    printf("  --debug           Enable debug output\n");
    printf("\nExamples:\n");
    printf("  %s input.pcc output.json\n", program_name);
    printf("  %s -f text input.pcc\n", program_name);
    printf("  %s -O input.pcc output.json\n", program_name);
}

/* Print version information */
static void print_version(void) {
    printf("PCC Compiler v%s\n", PCC_VERSION);
    printf("A lightweight compiler for converting DSL to LLM prompts\n");
}

/* Read file contents */
static char* read_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return NULL;
    }

    /* Get file size */
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    /* Allocate buffer */
    char* buffer = (char*)malloc(size + 1);
    if (buffer == NULL) {
        fclose(file);
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }

    /* Read file */
    size_t read = fread(buffer, 1, size, file);
    buffer[read] = '\0';
    fclose(file);

    return buffer;
}

/* Main function */
int main(int argc, char* argv[]) {
    const char* input_file = NULL;
    const char* output_file = NULL;
    OutputFormat format = OUTPUT_FORMAT_JSON;
    int optimize = 0;
    int debug = 0;

    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                output_file = argv[++i];
            } else {
                fprintf(stderr, "Error: -o requires an argument\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-f") == 0) {
            if (i + 1 < argc) {
                const char* fmt = argv[++i];
                if (strcmp(fmt, "json") == 0) {
                    format = OUTPUT_FORMAT_JSON;
                } else if (strcmp(fmt, "text") == 0) {
                    format = OUTPUT_FORMAT_TEXT;
                } else if (strcmp(fmt, "markdown") == 0) {
                    format = OUTPUT_FORMAT_MARKDOWN;
                } else {
                    fprintf(stderr, "Error: Unknown format '%s'\n", fmt);
                    return 1;
                }
            } else {
                fprintf(stderr, "Error: -f requires an argument\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-O") == 0) {
            optimize = 1;
        } else if (strcmp(argv[i], "--no-optimize") == 0) {
            optimize = 0;
        } else if (strcmp(argv[i], "--debug") == 0) {
            debug = 1;
        } else if (argv[i][0] != '-') {
            /* Handle positional arguments: input_file and output_file */
            if (input_file == NULL) {
                input_file = argv[i];
            } else if (output_file == NULL) {
                output_file = argv[i];
            } else {
                fprintf(stderr, "Error: Too many arguments\n");
                return 1;
            }
        } else {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            return 1;
        }
    }

    /* Check if input file is provided */
    if (input_file == NULL) {
        fprintf(stderr, "Error: No input file specified\n");
        print_usage(argv[0]);
        return 1;
    }

    /* Set default output file if not specified */
    if (output_file == NULL) {
        output_file = "outputs/output.json";
    }

    if (debug) {
        printf("PCC Compiler v%s\n", PCC_VERSION);
        printf("Input: %s\n", input_file);
        printf("Output: %s\n", output_file);
        printf("Format: %s\n", format == OUTPUT_FORMAT_JSON ? "JSON" : 
               format == OUTPUT_FORMAT_TEXT ? "TEXT" : "MARKDOWN");
        printf("Optimizations: %s\n", optimize ? "Enabled" : "Disabled");
        printf("\n");
    }

    /* Read input file */
    char* source = read_file(input_file);
    if (source == NULL) {
        return 1;
    }

    /* Lexical analysis */
    if (debug) printf("Lexical analysis...\n");
    Lexer* lexer = lexer_create(source, input_file);
    if (lexer == NULL) {
        free(source);
        fprintf(stderr, "Error: Failed to create lexer\n");
        return 1;
    }

    PCCError err = lexer_tokenize(lexer);
    if (err != PCC_SUCCESS) {
        fprintf(stderr, "Error: Lexical analysis failed\n");
        lexer_free(lexer);
        free(source);
        return 1;
    }

    if (debug) {
        printf("  Tokens: %zu\n", lexer_token_count(lexer));
    }

    /* Parsing */
    if (debug) printf("Parsing...\n");
    Parser* parser = parser_create(lexer);
    if (parser == NULL) {
        lexer_free(lexer);
        free(source);
        fprintf(stderr, "Error: Failed to create parser\n");
        return 1;
    }

    ASTNode* ast = parser_parse(parser);
    if (ast == NULL || parser_has_errors(parser)) {
        fprintf(stderr, "Error: Parsing failed\n");
        parser_print_errors(parser);
        parser_free(parser);
        lexer_free(lexer);
        free(source);
        return 1;
    }

    if (debug) {
        printf("  AST created successfully\n");
    }

    /* Semantic analysis */
    if (debug) printf("Semantic analysis...\n");
    SemanticAnalyzer* semantic = semantic_analyzer_create();
    if (semantic == NULL) {
        ast_node_free(ast);
        parser_free(parser);
        lexer_free(lexer);
        free(source);
        fprintf(stderr, "Error: Failed to create semantic analyzer\n");
        return 1;
    }

    err = semantic_analyze(semantic, ast);
    if (err != PCC_SUCCESS || semantic_has_errors(semantic)) {
        fprintf(stderr, "Error: Semantic analysis failed\n");
        semantic_print_errors(semantic);
        semantic_analyzer_free(semantic);
        ast_node_free(ast);
        parser_free(parser);
        lexer_free(lexer);
        free(source);
        return 1;
    }

    if (debug) {
        printf("  Semantic analysis passed\n");
    }

    /* Optimization */
    if (optimize) {
        if (debug) printf("Optimization...\n");
        Optimizer* optimizer = optimizer_create(1 << OPT_ALL);
        if (optimizer != NULL) {
            ast = optimizer_optimize(optimizer, ast);
            if (debug) {
                printf("  Optimizations applied: %d\n",
                       optimizer_get_optimizations_applied(optimizer));
            }
            optimizer_free(optimizer);
        }
    }

    /* Code generation */
    if (debug) printf("Code generation...\n");
    CodeGenerator* codegen = codegen_create(format, semantic_get_symbol_table(semantic));
    if (codegen == NULL) {
        semantic_analyzer_free(semantic);
        ast_node_free(ast);
        parser_free(parser);
        lexer_free(lexer);
        free(source);
        fprintf(stderr, "Error: Failed to create code generator\n");
        return 1;
    }

    err = codegen_generate(codegen, ast);
    if (err != PCC_SUCCESS) {
        fprintf(stderr, "Error: Code generation failed\n");
        codegen_free(codegen);
        semantic_analyzer_free(semantic);
        ast_node_free(ast);
        parser_free(parser);
        lexer_free(lexer);
        free(source);
        return 1;
    }

    /* Write output to file */
    err = codegen_write_to_file(codegen, output_file);
    if (err != PCC_SUCCESS) {
        fprintf(stderr, "Error: Failed to write output file\n");
        codegen_free(codegen);
        semantic_analyzer_free(semantic);
        ast_node_free(ast);
        parser_free(parser);
        lexer_free(lexer);
        free(source);
        return 1;
    }

    if (debug) {
        printf("  Output written to: %s\n", output_file);
    }

    /* Cleanup */
    if (debug) printf("Cleanup: freeing codegen...\n");
    codegen_free(codegen);
    if (debug) printf("Cleanup: freeing ast...\n");
    ast_node_free(ast);
    if (debug) printf("Cleanup: freeing semantic analyzer...\n");
    semantic_analyzer_free(semantic);
    if (debug) printf("Cleanup: freeing parser...\n");
    parser_free(parser);
    if (debug) printf("Cleanup: freeing lexer...\n");
    lexer_free(lexer);
    if (debug) printf("Cleanup: freeing source...\n");
    free(source);
    if (debug) printf("Cleanup: done\n");

    printf("Compilation successful!\n");
    printf("Output: %s\n", output_file);

    return 0;
}
