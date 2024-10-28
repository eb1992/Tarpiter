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
#define N_LINES_IN_DEBUG 17

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

typedef struct {
  unsigned char *cells;
  unsigned char *cur_cell;
  char *output_buffer;
  char *output_buffer_ptr;
  bool restart;
  bool debug;
  size_t skip;
  size_t ticks;
  size_t instr_ptr;
  Token *tokens;
  size_t n_tokens;
} BF_state;

static void process_arguments(int argc, char **argv, bool *debug);
static void tokenize_file(FILE *file, Token tokens[], size_t *n_tokens);
static void optimize(Token tokens[], size_t *n_tokens);
static void calculate_jumps(Token tokens[], size_t n_tokens);
static void evaluate_tokens(Token tokens[], size_t n_tokens, bool debug);
static void evaluate_token(BF_state *state);
static bool is_valid(int chr);
static FILE *open_file(const char *file_name);
static size_t get_file_size(FILE *file);
static size_t get_terminal_width();
static void print_usage(void);
static void handle_user_input(BF_state *state);
static void print_state(BF_state *state);
static void append_program(BF_state *state, size_t term_width, char **debug_buffer_ptr);
static void append_cells(BF_state *state, size_t term_width, char **debug_buffer_ptr);
// static void append_cells(const unsigned char *cells,
                        //  const unsigned char *cur_cell, size_t term_width,
                        //  char **debug_buffer_ptr);
static void append_pointer(size_t steps, size_t step_size, char **debug_buffer_ptr);
static void clear_terminal(void);

#endif // TARPITER_H