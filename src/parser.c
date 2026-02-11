/**
 * @file parser.c
 * @brief Parser implementation for PCC compiler
 */

#include "parser.h"
#include <stdio.h>
#include <string.h>

/* Forward declarations for static functions */
static ASTNode* parse_program(Parser* parser);
static ASTNode* parse_statement(Parser* parser);
static ASTNode* parse_prompt_def(Parser* parser);
static ASTNode* parse_var_decl(Parser* parser);
static ASTNode* parse_template_def(Parser* parser);
static ASTNode* parse_constraint_def(Parser* parser);
static ASTNode* parse_output_spec(Parser* parser);
static ASTNode* parse_element_list(Parser* parser);
static ASTNode* parse_element(Parser* parser);
static ASTNode* parse_text_element(Parser* parser, int is_raw);
static ASTNode* parse_variable_element(Parser* parser);
static ASTNode* parse_template_element(Parser* parser);
static ASTNode* parse_if_stmt(Parser* parser);
static ASTNode* parse_for_stmt(Parser* parser);
static ASTNode* parse_while_stmt(Parser* parser);
static ASTNode* parse_expression(Parser* parser);
static ASTNode* parse_logical_or(Parser* parser);
static ASTNode* parse_logical_and(Parser* parser);
static ASTNode* parse_equality(Parser* parser);
static ASTNode* parse_comparison(Parser* parser);
static ASTNode* parse_term(Parser* parser);
static ASTNode* parse_factor(Parser* parser);
static ASTNode* parse_unary(Parser* parser);
static ASTNode* parse_power(Parser* parser);
static ASTNode* parse_primary(Parser* parser);
static ASTNode* parse_constraint_expr(Parser* parser);

/* Helper functions */

static Token* parser_peek(Parser* parser, size_t offset) {
    if (parser == NULL || parser->lexer == NULL) {
        return NULL;
    }
    return lexer_get_token(parser->lexer, parser->current + offset);
}

static Token* parser_current(Parser* parser) {
    return parser_peek(parser, 0);
}

static Token* parser_advance(Parser* parser) {
    Token* token = parser_current(parser);
    if (token != NULL && token->type != TOKEN_EOF) {
        parser->current++;
    }
    return token;
}

static PCCBool parser_check(Parser* parser, TokenType type) {
    Token* token = parser_current(parser);
    return (PCCBool)(token != NULL && token->type == type);
}

static PCCBool parser_match(Parser* parser, TokenType type) {
    if (parser_check(parser, type)) {
        parser_advance(parser);
        return PCC_TRUE;
    }
    return PCC_FALSE;
}

static void parser_error(Parser* parser, const char* message) {
    Token* token = parser_current(parser);
    ParseError* error = (ParseError*)malloc(sizeof(ParseError));
    if (error == NULL) {
        return;
    }

    error->message = pcc_strdup(message);
    if (token != NULL) {
        error->position = token->position;
    } else {
        error->position.line = 0;
        error->position.column = 0;
        error->position.filename = "<unknown>";
    }

    pcc_array_push(parser->errors, error);
}

static Token* parser_consume(Parser* parser, TokenType type, const char* error_msg) {
    if (parser_check(parser, type)) {
        return parser_advance(parser);
    }
    parser_error(parser, error_msg);
    return NULL;
}

/* Parsing functions */

/* Parse a program (list of statements) */
static ASTNode* parse_program(Parser* parser) {
    PCCArray* statements = pcc_array_create(INITIAL_CAPACITY, sizeof(ASTNode*));
    if (statements == NULL) {
        return NULL;
    }

    while (!parser_check(parser, TOKEN_EOF)) {
        ASTNode* stmt = parse_statement(parser);
        if (stmt != NULL) {
            pcc_array_push(statements, stmt);
        } else {
            /* Skip to next statement on error */
            while (!parser_check(parser, TOKEN_EOF) &&
                   !parser_check(parser, TOKEN_PROMPT) &&
                   !parser_check(parser, TOKEN_VAR) &&
                   !parser_check(parser, TOKEN_TEMPLATE) &&
                   !parser_check(parser, TOKEN_CONSTRAINT) &&
                   !parser_check(parser, TOKEN_OUTPUT)) {
                parser_advance(parser);
            }
        }
    }

    return ast_program_create(statements);
}

/* Parse a statement */
static ASTNode* parse_statement(Parser* parser) {
    Token* token = parser_current(parser);

    if (token == NULL) {
        return NULL;
    }

    switch (token->type) {
        case TOKEN_PROMPT:
            return parse_prompt_def(parser);
        case TOKEN_VAR:
            return parse_var_decl(parser);
        case TOKEN_TEMPLATE:
            return parse_template_def(parser);
        case TOKEN_CONSTRAINT:
            return parse_constraint_def(parser);
        case TOKEN_OUTPUT:
            return parse_output_spec(parser);
        default:
            parser_error(parser, "Expected statement");
            return NULL;
    }
}

/* Parse prompt definition */
static ASTNode* parse_prompt_def(Parser* parser) {
    Token* prompt_token = parser_consume(parser, TOKEN_PROMPT, "Expected PROMPT keyword");
    if (prompt_token == NULL) return NULL;

    Token* name_token = parser_consume(parser, TOKEN_IDENTIFIER, "Expected prompt name");
    if (name_token == NULL) return NULL;

    parser_consume(parser, TOKEN_LBRACE, "Expected '{' after prompt name");

    ASTNode* body = parse_element_list(parser);

    parser_consume(parser, TOKEN_RBRACE, "Expected '}' after prompt body");

    return ast_prompt_def_create(name_token->lexeme, body, prompt_token->position);
}

/* Parse variable declaration */
static ASTNode* parse_var_decl(Parser* parser) {
    Token* var_token = parser_consume(parser, TOKEN_VAR, "Expected VAR keyword");
    if (var_token == NULL) return NULL;

    Token* name_token = parser_consume(parser, TOKEN_IDENTIFIER, "Expected variable name");
    if (name_token == NULL) return NULL;

    parser_consume(parser, TOKEN_ASSIGN, "Expected '=' after variable name");

    ASTNode* initializer = parse_expression(parser);
    if (initializer == NULL) {
        parser_error(parser, "Expected expression after '='");
        return NULL;
    }

    parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' after variable declaration");

    return ast_var_decl_create(name_token->lexeme, initializer, var_token->position);
}

/* Parse template definition */
static ASTNode* parse_template_def(Parser* parser) {
    Token* template_token = parser_consume(parser, TOKEN_TEMPLATE, "Expected TEMPLATE keyword");
    if (template_token == NULL) return NULL;

    Token* name_token = parser_consume(parser, TOKEN_IDENTIFIER, "Expected template name");
    if (name_token == NULL) return NULL;

    parser_consume(parser, TOKEN_LPAREN, "Expected '(' after template name");

    PCCArray* parameters = pcc_array_create(INITIAL_CAPACITY, sizeof(char*));
    if (parameters == NULL) return NULL;

    if (!parser_check(parser, TOKEN_RPAREN)) {
        do {
            Token* param_token = parser_consume(parser, TOKEN_IDENTIFIER, "Expected parameter name");
            if (param_token != NULL) {
                pcc_array_push(parameters, pcc_strdup(param_token->lexeme));
            }
        } while (parser_match(parser, TOKEN_COMMA));
    }

    parser_consume(parser, TOKEN_RPAREN, "Expected ')' after parameters");

    parser_consume(parser, TOKEN_LBRACE, "Expected '{' after template parameters");

    ASTNode* body = parse_element_list(parser);

    parser_consume(parser, TOKEN_RBRACE, "Expected '}' after template body");

    return ast_template_def_create(name_token->lexeme, parameters, body, template_token->position);
}

/* Parse constraint definition */
static ASTNode* parse_constraint_def(Parser* parser) {
    Token* constraint_token = parser_consume(parser, TOKEN_CONSTRAINT, "Expected CONSTRAINT keyword");
    if (constraint_token == NULL) return NULL;

    Token* name_token = parser_consume(parser, TOKEN_IDENTIFIER, "Expected constraint name");
    if (name_token == NULL) return NULL;

    parser_consume(parser, TOKEN_LBRACE, "Expected '{' after constraint name");

    PCCArray* constraints = pcc_array_create(INITIAL_CAPACITY, sizeof(ASTNode*));
    if (constraints == NULL) return NULL;

    while (!parser_check(parser, TOKEN_RBRACE) && !parser_check(parser, TOKEN_EOF)) {
        ASTNode* constraint = parse_constraint_expr(parser);
        if (constraint != NULL) {
            pcc_array_push(constraints, constraint);
        }
        parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' after constraint");
    }

    parser_consume(parser, TOKEN_RBRACE, "Expected '}' after constraint body");

    return ast_constraint_def_create(name_token->lexeme, constraints, constraint_token->position);
}

/* Parse constraint expression */
static ASTNode* parse_constraint_expr(Parser* parser) {
    Token* var_token = parser_consume(parser, TOKEN_IDENTIFIER, "Expected variable name");
    if (var_token == NULL) return NULL;

    Token* op_token = parser_current(parser);
    int op_type = 0;

    switch (op_token->type) {
        case TOKEN_EQ: op_type = TOKEN_EQ; break;
        case TOKEN_NE: op_type = TOKEN_NE; break;
        case TOKEN_LT: op_type = TOKEN_LT; break;
        case TOKEN_GT: op_type = TOKEN_GT; break;
        case TOKEN_LE: op_type = TOKEN_LE; break;
        case TOKEN_GE: op_type = TOKEN_GE; break;
        case TOKEN_IN: op_type = TOKEN_IN_OP; break;
        default:
            parser_error(parser, "Expected comparison operator");
            return NULL;
    }

    parser_advance(parser);

    ASTNode* value = parse_expression(parser);
    if (value == NULL) {
        parser_error(parser, "Expected value after operator");
        return NULL;
    }

    return ast_constraint_expr_create(var_token->lexeme, op_type, value, var_token->position);
}

/* Parse output specification */
static ASTNode* parse_output_spec(Parser* parser) {
    Token* output_token = parser_consume(parser, TOKEN_OUTPUT, "Expected OUTPUT keyword");
    if (output_token == NULL) return NULL;

    Token* name_token = parser_consume(parser, TOKEN_IDENTIFIER, "Expected output name");
    if (name_token == NULL) return NULL;

    parser_consume(parser, TOKEN_AS, "Expected AS keyword");

    Token* format_token = parser_current(parser);
    int format = 0;

    if (parser_match(parser, TOKEN_IDENTIFIER)) {
        if (strcmp(format_token->lexeme, "JSON") == 0) {
            format = 1;
        } else if (strcmp(format_token->lexeme, "TEXT") == 0) {
            format = 2;
        } else if (strcmp(format_token->lexeme, "MARKDOWN") == 0) {
            format = 3;
        } else {
            parser_error(parser, "Expected format (JSON, TEXT, or MARKDOWN)");
        }
    }

    parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' after output specification");

    return ast_output_spec_create(name_token->lexeme, format, output_token->position);
}

/* Parse element list */
static ASTNode* parse_element_list(Parser* parser) {
    PCCArray* elements = pcc_array_create(INITIAL_CAPACITY, sizeof(ASTNode*));
    if (elements == NULL) return NULL;

    while (!parser_check(parser, TOKEN_RBRACE) && !parser_check(parser, TOKEN_RPAREN) &&
           !parser_check(parser, TOKEN_EOF)) {
        ASTNode* element = parse_element(parser);
        if (element != NULL) {
            pcc_array_push(elements, element);
        }
    }

    PCCPosition pos = {0, 0, "<element_list>"};
    return ast_list_create(AST_ELEMENT_LIST, elements, pos);
}

/* Parse element */
static ASTNode* parse_element(Parser* parser) {
    Token* token = parser_current(parser);

    if (token == NULL) {
        return NULL;
    }

    switch (token->type) {
        case TOKEN_STRING:
            return parse_text_element(parser, 0);
        case TOKEN_RAW:
            return parse_text_element(parser, 1);
        case TOKEN_VARIABLE_REF:
            return parse_variable_element(parser);
        case TOKEN_TEMPLATE_CALL:
            return parse_template_element(parser);
        case TOKEN_IF:
            return parse_if_stmt(parser);
        case TOKEN_FOR:
            return parse_for_stmt(parser);
        case TOKEN_WHILE:
            return parse_while_stmt(parser);
        default:
            /* Try to parse as expression */
            return parse_expression(parser);
    }
}

/* Parse text element */
static ASTNode* parse_text_element(Parser* parser, int is_raw) {
    PCCPosition pos;

    if (is_raw) {
        pos = parser_current(parser)->position;
    } else {
        pos = parser_current(parser)->position;
    }

    Token* string_token = parser_consume(parser, TOKEN_STRING, "Expected string literal");
    if (string_token == NULL) return NULL;

    return ast_text_element_create(string_token->value.string_val, is_raw, pos);
}

/* Parse variable element */
static ASTNode* parse_variable_element(Parser* parser) {
    Token* token = parser_consume(parser, TOKEN_VARIABLE_REF, "Expected variable reference");
    if (token == NULL) return NULL;

    /* Extract variable name from lexeme (skip $) */
    char* var_name = pcc_strdup(token->lexeme + 1);
    if (var_name == NULL) return NULL;

    ASTNode* node = ast_variable_ref_create(var_name, token->position);
    free(var_name);

    return node;
}

/* Parse template element */
static ASTNode* parse_template_element(Parser* parser) {
    Token* token = parser_consume(parser, TOKEN_TEMPLATE_CALL, "Expected template call");
    if (token == NULL) return NULL;

    parser_consume(parser, TOKEN_LPAREN, "Expected '(' after template name");

    PCCArray* arguments = pcc_array_create(INITIAL_CAPACITY, sizeof(ASTNode*));
    if (arguments == NULL) return NULL;

    if (!parser_check(parser, TOKEN_RPAREN)) {
        do {
            ASTNode* arg = parse_expression(parser);
            if (arg != NULL) {
                pcc_array_push(arguments, arg);
            }
        } while (parser_match(parser, TOKEN_COMMA));
    }

    parser_consume(parser, TOKEN_RPAREN, "Expected ')' after template arguments");

    /* Extract template name from lexeme (skip @) */
    char* template_name = pcc_strdup(token->lexeme + 1);
    if (template_name == NULL) {
        pcc_array_free(arguments, NULL);
        return NULL;
    }

    ASTNode* node = ast_function_call_create(template_name, arguments, token->position);
    free(template_name);

    return node;
}

/* Parse if statement */
static ASTNode* parse_if_stmt(Parser* parser) {
    Token* if_token = parser_consume(parser, TOKEN_IF, "Expected IF keyword");
    if (if_token == NULL) return NULL;

    ASTNode* condition = parse_expression(parser);
    if (condition == NULL) {
        parser_error(parser, "Expected condition after IF");
        return NULL;
    }

    parser_consume(parser, TOKEN_LBRACE, "Expected '{' after IF condition");

    ASTNode* then_body = parse_element_list(parser);

    parser_consume(parser, TOKEN_RBRACE, "Expected '}' after IF body");

    ASTNode* else_body = NULL;
    if (parser_match(parser, TOKEN_ELSE)) {
        parser_consume(parser, TOKEN_LBRACE, "Expected '{' after ELSE");
        else_body = parse_element_list(parser);
        parser_consume(parser, TOKEN_RBRACE, "Expected '}' after ELSE body");
    }

    return ast_if_stmt_create(condition, then_body, else_body, if_token->position);
}

/* Parse for statement */
static ASTNode* parse_for_stmt(Parser* parser) {
    Token* for_token = parser_consume(parser, TOKEN_FOR, "Expected FOR keyword");
    if (for_token == NULL) return NULL;

    Token* var_token = parser_consume(parser, TOKEN_IDENTIFIER, "Expected loop variable");
    if (var_token == NULL) return NULL;

    parser_consume(parser, TOKEN_IN, "Expected IN keyword");

    ASTNode* iterable = parse_expression(parser);
    if (iterable == NULL) {
        parser_error(parser, "Expected iterable after IN");
        return NULL;
    }

    parser_consume(parser, TOKEN_LBRACE, "Expected '{' after FOR iterable");

    ASTNode* body = parse_element_list(parser);

    parser_consume(parser, TOKEN_RBRACE, "Expected '}' after FOR body");

    return ast_for_stmt_create(var_token->lexeme, iterable, body, for_token->position);
}

/* Parse while statement */
static ASTNode* parse_while_stmt(Parser* parser) {
    Token* while_token = parser_consume(parser, TOKEN_WHILE, "Expected WHILE keyword");
    if (while_token == NULL) return NULL;

    ASTNode* condition = parse_expression(parser);
    if (condition == NULL) {
        parser_error(parser, "Expected condition after WHILE");
        return NULL;
    }

    parser_consume(parser, TOKEN_LBRACE, "Expected '{' after WHILE condition");

    ASTNode* body = parse_element_list(parser);

    parser_consume(parser, TOKEN_RBRACE, "Expected '}' after WHILE body");

    return ast_while_stmt_create(condition, body, while_token->position);
}

/* Parse expression (lowest precedence) */
static ASTNode* parse_expression(Parser* parser) {
    return parse_logical_or(parser);
}

/* Parse logical OR */
static ASTNode* parse_logical_or(Parser* parser) {
    ASTNode* left = parse_logical_and(parser);

    while (parser_match(parser, TOKEN_OR)) {
        Token* op_token = parser_current(parser);
        ASTNode* right = parse_logical_and(parser);
        left = ast_binary_expr_create(TOKEN_OR, left, right, op_token->position);
    }

    return left;
}

/* Parse logical AND */
static ASTNode* parse_logical_and(Parser* parser) {
    ASTNode* left = parse_equality(parser);

    while (parser_match(parser, TOKEN_AND)) {
        Token* op_token = parser_current(parser);
        ASTNode* right = parse_equality(parser);
        left = ast_binary_expr_create(TOKEN_AND, left, right, op_token->position);
    }

    return left;
}

/* Parse equality */
static ASTNode* parse_equality(Parser* parser) {
    ASTNode* left = parse_comparison(parser);

    while (parser_match(parser, TOKEN_EQ) || parser_match(parser, TOKEN_NE)) {
        Token* op_token = parser_current(parser);
        ASTNode* right = parse_comparison(parser);
        left = ast_binary_expr_create(op_token->type, left, right, op_token->position);
    }

    return left;
}

/* Parse comparison */
static ASTNode* parse_comparison(Parser* parser) {
    ASTNode* left = parse_term(parser);

    while (parser_match(parser, TOKEN_LT) || parser_match(parser, TOKEN_GT) ||
           parser_match(parser, TOKEN_LE) || parser_match(parser, TOKEN_GE)) {
        Token* op_token = parser_current(parser);
        ASTNode* right = parse_term(parser);
        left = ast_binary_expr_create(op_token->type, left, right, op_token->position);
    }

    return left;
}

/* Parse term (addition/subtraction) */
static ASTNode* parse_term(Parser* parser) {
    ASTNode* left = parse_factor(parser);

    while (parser_match(parser, TOKEN_ADD) || parser_match(parser, TOKEN_SUB)) {
        Token* op_token = parser_current(parser);
        ASTNode* right = parse_factor(parser);
        left = ast_binary_expr_create(op_token->type, left, right, op_token->position);
    }

    return left;
}

/* Parse factor (multiplication/division/modulo) */
static ASTNode* parse_factor(Parser* parser) {
    ASTNode* left = parse_power(parser);

    while (parser_match(parser, TOKEN_MUL) || parser_match(parser, TOKEN_DIV) ||
           parser_match(parser, TOKEN_MOD)) {
        Token* op_token = parser_current(parser);
        ASTNode* right = parse_power(parser);
        left = ast_binary_expr_create(op_token->type, left, right, op_token->position);
    }

    return left;
}

/* Parse power */
static ASTNode* parse_power(Parser* parser) {
    ASTNode* left = parse_unary(parser);

    if (parser_match(parser, TOKEN_POW)) {
        Token* op_token = parser_current(parser);
        ASTNode* right = parse_power(parser); /* Right associative */
        left = ast_binary_expr_create(TOKEN_POW, left, right, op_token->position);
    }

    return left;
}

/* Parse unary expression */
static ASTNode* parse_unary(Parser* parser) {
    if (parser_match(parser, TOKEN_NOT) || parser_match(parser, TOKEN_SUB)) {
        Token* op_token = parser_current(parser);
        ASTNode* operand = parse_unary(parser);
        return ast_unary_expr_create(op_token->type, operand, op_token->position);
    }

    return parse_primary(parser);
}

/* Parse primary expression */
static ASTNode* parse_primary(Parser* parser) {
    Token* token = parser_current(parser);

    if (token == NULL) {
        return NULL;
    }

    switch (token->type) {
        case TOKEN_IDENTIFIER: {
            Token* id_token = parser_advance(parser);
            return ast_identifier_create(id_token->lexeme, id_token->position);
        }
        case TOKEN_STRING: {
            Token* str_token = parser_advance(parser);
            return ast_string_literal_create(str_token->value.string_val, str_token->position);
        }
        case TOKEN_NUMBER: {
            Token* num_token = parser_advance(parser);
            return ast_number_literal_create(num_token->value.number_val, num_token->position);
        }
        case TOKEN_TRUE: {
            Token* bool_token = parser_advance(parser);
            return ast_boolean_literal_create(1, bool_token->position);
        }
        case TOKEN_FALSE: {
            Token* bool_token = parser_advance(parser);
            return ast_boolean_literal_create(0, bool_token->position);
        }
        case TOKEN_LPAREN: {
            parser_advance(parser);
            ASTNode* expr = parse_expression(parser);
            parser_consume(parser, TOKEN_RPAREN, "Expected ')' after expression");
            return expr;
        }
        default:
            parser_error(parser, "Expected expression");
            return NULL;
    }
}

/* Public functions */

Parser* parser_create(Lexer* lexer) {
    if (lexer == NULL) {
        return NULL;
    }

    Parser* parser = (Parser*)malloc(sizeof(Parser));
    if (parser == NULL) {
        return NULL;
    }

    parser->lexer = lexer;
    parser->current = 0;
    parser->errors = pcc_array_create(INITIAL_CAPACITY, sizeof(ParseError*));
    parser->error_message = NULL;

    if (parser->errors == NULL) {
        free(parser);
        return NULL;
    }

    return parser;
}

void parser_free(Parser* parser) {
    if (parser == NULL) {
        return;
    }

    /* Free all errors */
    for (size_t i = 0; i < pcc_array_size(parser->errors); i++) {
        ParseError* error = (ParseError*)pcc_array_get(parser->errors, i);
        if (error != NULL) {
            if (error->message != NULL) {
                free(error->message);
            }
            free(error);
        }
    }

    pcc_array_free(parser->errors, NULL);

    if (parser->error_message != NULL) {
        free(parser->error_message);
    }

    free(parser);
}

ASTNode* parser_parse(Parser* parser) {
    if (parser == NULL) {
        return NULL;
    }

    return parse_program(parser);
}

size_t parser_error_count(Parser* parser) {
    if (parser == NULL) {
        return 0;
    }
    return pcc_array_size(parser->errors);
}

ParseError* parser_get_error(Parser* parser, size_t index) {
    if (parser == NULL) {
        return NULL;
    }
    return (ParseError*)pcc_array_get(parser->errors, index);
}

const char* parser_get_error_message(Parser* parser) {
    if (parser == NULL || pcc_array_size(parser->errors) == 0) {
        return NULL;
    }
    ParseError* error = (ParseError*)pcc_array_get(parser->errors, pcc_array_size(parser->errors) - 1);
    return error ? error->message : NULL;
}

void parser_print_errors(Parser* parser) {
    if (parser == NULL) {
        return;
    }

    for (size_t i = 0; i < pcc_array_size(parser->errors); i++) {
        ParseError* error = (ParseError*)pcc_array_get(parser->errors, i);
        if (error != NULL) {
            printf("Error at line %d, column %d: %s\n",
                   error->position.line, error->position.column, error->message);
        }
    }
}

PCCBool parser_has_errors(Parser* parser) {
    return (PCCBool)(parser != NULL && pcc_array_size(parser->errors) > 0);
}
