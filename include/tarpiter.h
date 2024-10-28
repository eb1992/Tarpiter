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
  Token *list;
  size_t count;
} Tokens;

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
} Eval_state;

static void process_arguments(int argc, char **argv, bool *debug);
static void tokenize_file(FILE *file, Tokens *tokens);
static void optimize(Tokens *tokens);
static void calculate_jumps(Tokens *tokens);
static void evaluate_tokens(Tokens *tokens, bool debug);
static void evaluate_token(Token token, Eval_state *state);
static bool is_valid(int chr);
static FILE *open_file(const char *file_name);
static size_t get_file_size(FILE *file);
static size_t get_terminal_width(void);
static void print_usage(void);
static void handle_user_input(Eval_state *state);
static void print_state(Eval_state *state, Tokens *tokens);
static void append_program(Eval_state *state, Tokens *tokens, size_t term_width,
                           char **debug_buffer_ptr);
static void append_cells(Eval_state *state, size_t term_width,
                         char **debug_buffer_ptr);
static void append_pointer(size_t steps, size_t step_size,
                           char **debug_buffer_ptr);
static void clear_terminal(void);

#endif // TARPITER_H