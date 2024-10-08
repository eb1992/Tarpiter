// TODO: update var names in signatures
#ifndef TARPITER_H

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define OUTPUT_BUFFER_SIZE 4096
#define N_CELLS 65536
#define SHOWN_CELL_WIDTH 5
#define MIN_ROW_SIZE 40

typedef enum {
  INCR = '+',
  DECR = '-',
  LEFT = '<',
  RIGHT = '>',
  JMP_F = '[',
  JMP_B = ']',
  PRINT = '.',
  INPUT = ','
} Operator;

typedef struct {
  Operator op;
  union {
    size_t address;
    size_t amount;
  };

} Token;

static void process_arguments(int argc, char **argv, bool *debug);
static void tokenize_file(FILE *file, Token tokens[], size_t *n_tokens);
static void optimize(Token tokens[], size_t *n_tokens);
static void handle_jumps(Token tokens[], size_t n_tokens);
static void evaluate_tokens(Token tokens[], size_t n_tokens, bool debug);
static void evaluate_token(Token token, unsigned char **cur_cell, char **bp,
                           char *output_buffer, bool debug, size_t *instr_ptr);
static bool is_valid(int c);
static FILE *open_file(const char *file_name);
static size_t get_file_size(FILE *file);
static size_t get_terminal_width();
static void print_usage(void);
static void handle_user_input(size_t *skip, size_t ticks, bool *restart);
static void print_state(const unsigned char cells[],
                        const unsigned char *cur_cell, const Token tokens[],
                        size_t n_tokens, size_t ticks, size_t token_index,
                        const char output_buffer[]);
static void append_cells(const unsigned char *cells,
                         const unsigned char *cur_cell, size_t term_width,
                         char **bp);
static void append_pointer(size_t steps, size_t step_size, char **bp);
static void append_program(const Token tokens[], size_t n_tokens,
                           size_t token_index, size_t term_width, char **bp);
static void clear_terminal(void);

#endif // TARPITER_H