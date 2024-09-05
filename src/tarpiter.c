/*
Beware of the Turing tar-pit in which everything is possible but nothing of
interest is easy.
  - Alan Perlis
*/

#include "../include/tarpiter.h"
#include <stddef.h>

int main(int argc, char **argv) {
  bool debug = false;

  process_arguments(argc, argv, &debug);

  FILE *file = open_file(argv[1]);

  Token tokens[get_file_size(file)];
  size_t n_tokens;

  tokenize_file(file, tokens, &n_tokens);

  fclose(file);

  if (!debug) {
    optimize(tokens, &n_tokens);
  }

  handle_jumps(tokens, n_tokens);

  evaluate_tokens(tokens, n_tokens, debug);

  return EXIT_SUCCESS;
}

// Process command-line arguments
static void process_arguments(int argc, char **argv, bool *debug) {
  if (argc < 2) {
    puts("ERROR: No file or flag provided.");
    print_usage();
    exit(EXIT_FAILURE);
  } else if (argc == 2 && ((strcmp(argv[1], "-h") == 0) ||
                           (strcmp(argv[1], "--help") == 0))) {
    print_usage();
    exit(EXIT_SUCCESS);

  } else if (argc > 2 && ((strcmp(argv[2], "-d") == 0) ||
                          (strcmp(argv[2], "--debug") == 0))) {
    *debug = true;
  }
}

// Tokenize the file content into tokens
static void tokenize_file(FILE *file, Token tokens[], size_t *n_tokens) {
  size_t n = 0;

  int c = fgetc(file);
  while (c != EOF) {
    if (is_valid(c)) {
      tokens[n++] = (Token){.op = c, .amount = 1};
    }
    c = fgetc(file);
  }
  if (ferror(file)) {
    fclose(file);
    fprintf(stderr, "ERROR: Could not read file.");
    exit(EXIT_FAILURE);
  }

  *n_tokens = n;
}

// Optimize the tokens by merging consecutive operations of the same type
static void optimize(Token tokens[], size_t *n_tokens) {
  size_t new_n_tokens = 0;
  size_t old_n_tokens = *n_tokens;

  for (size_t i = 0; i < old_n_tokens; i++) {
    tokens[new_n_tokens] = tokens[i];
    Operator op = tokens[i].op;
    if (op == INCR || op == DECR || op == RIGHT || op == LEFT) {
      size_t n_same_op = 1;
      while (i < old_n_tokens - 1 && tokens[i + 1].op == op) {
        n_same_op++;
        i++;
      }
      tokens[new_n_tokens].amount = n_same_op;
    }
    new_n_tokens++;
  }
  *n_tokens = new_n_tokens;
}

// Handle jump instructions by giving addresses to '[' and ']'
static void handle_jumps(Token tokens[], size_t n_tokens) {
  size_t stack[n_tokens];
  size_t stack_p = 0;

  for (size_t i = 0; i < n_tokens; i++) {
    Operator op = tokens[i].op;
    if (op == JMP_F) {
      stack[stack_p++] = i;
    } else if (op == JMP_B) {
      if (stack_p < 1) {
        fprintf(stderr, "ERROR: Unbalanced \'[]\' pair. A \'[\' is missing.\n");
        exit(EXIT_FAILURE);
      }
      size_t forward = stack[--stack_p];
      tokens[i].address = forward;
      tokens[forward].address = i;
    }
  }
  if (stack_p > 0) {
    fprintf(stderr, "ERROR: Unbalanced \'[]\' pair. A \']\' is missing.\n");
    exit(EXIT_FAILURE);
  }
}

// Evaluates the whole program
static void evaluate_tokens(Token tokens[], size_t n_tokens, bool debug) {

  unsigned char cells[N_CELLS];
  char output_buffer[OUTPUT_BUFFER_SIZE];
  bool restart;

  do {
    restart = false;
    memset(cells, 0, N_CELLS * sizeof(unsigned char));

    unsigned char *cur_cell = cells;
    char *bp = output_buffer;

    *bp = '\0';
    size_t skip = 0;
    size_t ticks = 0;

    for (size_t instr_ptr = 0; instr_ptr < n_tokens; instr_ptr++) {

      if (debug && skip <= ticks) {
        print_state(cells, cur_cell, tokens, n_tokens, ticks, instr_ptr,
                    output_buffer);
        handle_user_input(&skip, ticks, &restart);
        if (restart) {
          break;
        }
      }

      evaluate_token(tokens[instr_ptr], &cur_cell, &bp, output_buffer, debug,
                     &instr_ptr);
      ticks++;
    }
    if (!debug) {
      printf("%s", output_buffer);
    }
  } while (restart);
}

// Evaluate a single token
static void evaluate_token(Token token, unsigned char **cur_cell, char **bp,
                           char *output_buffer, bool debug, size_t *instr_ptr) {
  switch (token.op) {

  case LEFT:
    *cur_cell -= token.amount;
    break;

  case RIGHT:
    *cur_cell += token.amount;
    break;

  case DECR:
    **cur_cell -= (unsigned char)token.amount;
    break;

  case INCR:
    **cur_cell += (unsigned char)token.amount;
    break;

  case JMP_F:
    if (!**cur_cell) {
      *instr_ptr = token.address;
    }
    break;

  case JMP_B:
    if (**cur_cell) {
      *instr_ptr = token.address;
    }
    break;

  case PRINT: {
    if (*bp - output_buffer < OUTPUT_BUFFER_SIZE) {
      *(*bp)++ = (char)(**cur_cell == 10 ? '\n' : **cur_cell);
      **bp = '\0';
    } else {
      puts(output_buffer);
      *bp = output_buffer;
    }
    break;
  }

  case INPUT: {
    if (!debug) {
      puts(output_buffer);
      *bp = output_buffer;
    }

    int n = getchar();
    if (n != EOF) {
      **cur_cell = (unsigned char)(n == '\n' ? 10 : n);
    }
    break;
  }
  }
}

// Check if the character is a valid BF instruction
static bool is_valid(int c) {
  return c == INCR || c == DECR || c == LEFT || c == RIGHT || c == JMP_F ||
         c == JMP_B || c == PRINT || c == INPUT;
}

// Open the file for reading
static FILE *open_file(const char *file_name) {
  FILE *file = fopen(file_name, "r");
  if (file == NULL) {
    perror("ERROR: Could not open file");
    exit(EXIT_FAILURE);
  }
  return file;
}

// Get the size of the file
static size_t get_file_size(FILE *file) {
  fseek(file, 0, SEEK_END);
  size_t file_size = (size_t)ftell(file);
  fseek(file, 0, SEEK_SET);
  return file_size + 1;
}

// Get the width of the terminal
static size_t get_terminal_width() {
  struct winsize w;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) >= 0) {
    return w.ws_col;
  }
  return MIN_ROW_SIZE;
}

// Prints usage information
static void print_usage(void) {
  puts("Usage: tarpiter <file> [options]\n"
       "Options:\n"
       " -h, --help     Show this help message.\n"
       " -d, --debug    Run the program in debug mode.\n");
}

// Handle user input when debugging
static void handle_user_input(size_t *skip, size_t ticks, bool *restart) {
  char line[MIN_ROW_SIZE];
  size_t steps;
  if (fgets(line, sizeof(line), stdin)) {
    *line = (char)toupper(*line);
    if (0 == strcmp(line, "Q\n")) {
      exit(EXIT_SUCCESS);

    } else if (1 == sscanf(line, "%zu", &steps)) {
      *skip = steps + ticks;

    } else {
      *restart = 0 == strcmp(line, "R\n");
    }
  }
}

// Print the current state when debugging
static void print_state(const unsigned char cells[],
                        const unsigned char *cur_cell, const Token tokens[],
                        size_t n_tokens, size_t ticks, size_t token_index,
                        const char output_buffer[]) {

  size_t term_width = get_terminal_width();
  size_t n_lines = 15;
  char buffer[term_width * n_lines + n_lines];
  char *bp = buffer;

  clear_terminal();
  bp += sprintf(bp, "Evaluated instructions: %zu\n\n", ticks);

  append_program(tokens, n_tokens, token_index, term_width, &bp);
  append_cells(cells, cur_cell, term_width, &bp);
  bp += sprintf(bp, "[Enter]     - Evaluate single instruction.\n"
                    "<N> [Enter] - Evaluate <N> instructions.\n"
                    "[R]eset     - Reset the debugger.\n"
                    "[Q]uit      - Exit the debugger.\n\n"
                    "Program output:");
  *bp = '\0';

  puts(buffer);
  puts(output_buffer);
}

// Append the closest memory cells to the buffer
static void append_cells(const unsigned char *cells,
                         const unsigned char *cur_cell, size_t term_width,
                         char **bp) {
  const size_t cell_index = (size_t) (cur_cell - cells);
  const size_t half_row = term_width / SHOWN_CELL_WIDTH / 2;
  const size_t n_shown = term_width / SHOWN_CELL_WIDTH;
  const size_t first_cell = cell_index > half_row ? cell_index - half_row : 0;

  *bp += sprintf(*bp, "Cells:\n");

  for (size_t i = 0; i < n_shown; i++) {
    *bp += sprintf(*bp, "%3zu  ", (first_cell + i) % 1000);
  }
  *(*bp)++ = '\n';

  for (size_t i = 0; i < n_shown; i++) {
    unsigned char c =
        isprint(cells[first_cell + i]) ? cells[first_cell + i] : ' ';
    *bp += sprintf(*bp, "[ %c ]", c);
  }
  *(*bp)++ = '\n';

  for (size_t i = 0; i < n_shown; i++) {
    *bp += sprintf(*bp, "[%3d]", cells[first_cell + i]);
  }
  *(*bp)++ = '\n';

  append_pointer((size_t)(cur_cell - cells), SHOWN_CELL_WIDTH, bp);
  *(*bp)++ = '\n';
}

// Append a visual pointer to the buffer
static void append_pointer(size_t steps, size_t step_size, char **bp) {
  size_t n_chars = step_size * steps + step_size / 2;
  memset(*bp, ' ', n_chars);
  *(*bp + n_chars++) = '^';
  *(*bp + n_chars++) = '\n';
  *bp += n_chars;
}

// Append the closest part of the program to the buffer
static void append_program(const Token tokens[], size_t n_tokens,
                           size_t token_index, size_t term_width, char **bp) {
  size_t half = term_width / 2;
  size_t first_token = token_index > half ? token_index - half : 0;
  size_t i;

  for (i = 0; i < term_width && n_tokens > first_token + i; i++) {
    *(*bp)++ = tokens[first_token + i].op;
  }
  *(*bp)++ = '\n';

  append_pointer(token_index - first_token, 1, bp);
}

// Clear the terminal
static void clear_terminal(void) { printf("%s", "\x1B[1;1H\x1B[2J"); }