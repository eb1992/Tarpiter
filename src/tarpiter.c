#include "../include/tarpiter.h"

/*
Beware of the Turing tar-pit in which everything is possible but nothing of
interest is easy.
  - Alan Perlis
*/

int main(int argc, char **argv) {
  bool debug = false;

  process_arguments(argc, argv, &debug);

  FILE *file = open_file(argv[1]);

  Tokens tokens;

  tokens.list = malloc(get_file_size(file) * sizeof(Token));

  if (tokens.list == NULL) {
    fprintf(stderr, "Memory allocation failed");
    fclose(file);
    exit(EXIT_FAILURE);
  }

  tokenize_file(file, &tokens);

  fclose(file);

  if (!debug) {
    optimize(&tokens);
  }

  calculate_jumps(&tokens);

  evaluate_tokens(&tokens, debug);

  free(tokens.list);

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
static void tokenize_file(FILE *file, Tokens *tokens) {
  size_t n = 0;

  int c = fgetc(file);
  while (c != EOF) {
    if (is_valid(c)) {
      tokens->list[n++] = (Token){.op = c, .amount = 1};
    }
    c = fgetc(file);
  }
  if (ferror(file)) {
    fclose(file);
    fprintf(stderr, "ERROR: Could not read file.");
    exit(EXIT_FAILURE);
  }

  tokens->count = n;
}

// Optimize the tokens by merging consecutive operations of the same type
static void optimize(Tokens *tokens) {
  size_t new_n_tokens = 0;
  size_t old_n_tokens = tokens->count;

  for (size_t i = 0; i < old_n_tokens; i++) {
    tokens->list[new_n_tokens] = tokens->list[i];
    Operator op = tokens->list[i].op;
    if (op == INCR || op == DECR || op == RIGHT || op == LEFT) {
      size_t n_same_op = 1;
      while (i < old_n_tokens - 1 && tokens->list[i + 1].op == op) {
        n_same_op++;
        i++;
      }
      tokens->list[new_n_tokens].amount = n_same_op;
    }
    new_n_tokens++;
  }
  tokens->count = new_n_tokens;
}

// Handle jump instructions by giving addresses to '[' and ']'
static void calculate_jumps(Tokens *tokens) {
  size_t stack[tokens->count];
  size_t stack_p = 0;

  for (size_t i = 0; i < tokens->count; i++) {
    Operator op = tokens->list[i].op;
    if (op == JMP_F) {
      stack[stack_p++] = i;
    } else if (op == JMP_B) {
      if (stack_p < 1) {
        fprintf(stderr, "ERROR: Unbalanced \'[]\' pair. A \'[\' is missing.\n");
        exit(EXIT_FAILURE);
      }
      size_t forward = stack[--stack_p];
      tokens->list[i].address = forward;
      tokens->list[forward].address = i;
    }
  }
  if (stack_p > 0) {
    fprintf(stderr, "ERROR: Unbalanced \'[]\' pair. A \']\' is missing.\n");
    exit(EXIT_FAILURE);
  }
}

// Evaluates the whole program
static void evaluate_tokens(Tokens *tokens, bool debug) {
  Eval_state state;
  state.debug = debug;
  state.cells = malloc(N_CELLS * sizeof(unsigned char));
  state.output_buffer = malloc(OUTPUT_BUFFER_SIZE * sizeof(char));

  if (state.cells == NULL || state.output_buffer == NULL) {
    fprintf(stderr, "Memory allocation failed.");
    exit(EXIT_FAILURE);
  }

  do {
    state.restart = false;
    memset(state.cells, 0, N_CELLS * sizeof(unsigned char));

    state.cur_cell = state.cells;
    state.output_buffer_ptr = state.output_buffer;

    *state.output_buffer_ptr = '\0';
    state.skip = 0;
    state.ticks = 0;

    for (state.instr_ptr = 0; state.instr_ptr < tokens->count;
         state.instr_ptr++) {

      if (debug && state.skip <= state.ticks) {
        print_state(&state, tokens);
        handle_user_input(&state);
        if (state.restart) {
          break;
        }
      }

      evaluate_token(tokens->list[state.instr_ptr], &state);
      state.ticks++;
    }
    if (!debug) {
      printf("%s", state.output_buffer);
    }
  } while (state.restart);

  free(state.cells);
  free(state.output_buffer);
}

// Evaluate a single token
static void evaluate_token(Token token, Eval_state *state) {
  switch (token.op) {

  case LEFT:
    state->cur_cell -= token.amount;
    break;

  case RIGHT:
    state->cur_cell += token.amount;
    break;

  case DECR:
    *state->cur_cell -= (unsigned char)token.amount;
    break;

  case INCR:
    *state->cur_cell += (unsigned char)token.amount;
    break;

  case JMP_F:
    if (!*state->cur_cell) {
      state->instr_ptr = token.address;
    }
    break;

  case JMP_B:
    if (*state->cur_cell) {
      state->instr_ptr = token.address;
    }
    break;

  case PRINT: {
    if (state->output_buffer_ptr - state->output_buffer < OUTPUT_BUFFER_SIZE) {
      *(state->output_buffer_ptr)++ =
          (char)(*state->cur_cell == 10 ? '\n' : *state->cur_cell);
      *state->output_buffer_ptr = '\0';
    } else {
      puts(state->output_buffer);
      state->output_buffer_ptr = state->output_buffer;
    }
    break;
  }

  case INPUT: {
    if (!state->debug) {
      puts(state->output_buffer);
      state->output_buffer_ptr = state->output_buffer;
    }

    int n = getchar();
    if (n != EOF) {
      *state->cur_cell = (unsigned char)(n == '\n' ? 10 : n);
    }
    break;
  }
  }
}

// Check if the character is a valid BF instruction
static bool is_valid(int chr) { return strchr("+-<>[].,", chr); }

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
static void handle_user_input(Eval_state *state) {
  char line[MIN_ROW_SIZE];
  size_t steps;
  if (fgets(line, sizeof(line), stdin)) {
    *line = (char)toupper(*line);
    if (0 == strcmp(line, "Q\n")) {
      exit(EXIT_SUCCESS);

    } else if (1 == sscanf(line, "%zu", &steps)) {
      state->skip = steps + state->ticks;

    } else {
      state->restart = 0 == strcmp(line, "R\n");
    }
  }
}

// Print the current state when debugging
static void print_state(Eval_state *state, Tokens *tokens) {
  size_t term_width = get_terminal_width();
  char debug_buffer[term_width * N_LINES_IN_DEBUG];
  char *debug_buffer_ptr = debug_buffer;

  clear_terminal();
  debug_buffer_ptr += sprintf(debug_buffer_ptr,
                              "Evaluated instructions: %zu\n\n", state->ticks);

  append_program(state, tokens, term_width, &debug_buffer_ptr);
  append_cells(state, term_width, &debug_buffer_ptr);
  debug_buffer_ptr +=
      sprintf(debug_buffer_ptr, "[Enter]     - Evaluate single instruction.\n"
                                "<N> [Enter] - Evaluate <N> instructions.\n"
                                "[R]eset     - Reset the debugger.\n"
                                "[Q]uit      - Exit the debugger.\n\n"
                                "Program output:");
  *debug_buffer_ptr = '\0';

  puts(debug_buffer);
  puts(state->output_buffer);
}

// Append the closest part of the program to the buffer
static void append_program(Eval_state *state, Tokens *tokens, size_t term_width,
                           char **debug_buffer_ptr) {
  size_t half = term_width / 2;
  size_t first_token = state->instr_ptr > half ? state->instr_ptr - half : 0;
  size_t i;

  for (i = 0; i < term_width && tokens->count > first_token + i; i++) {
    *(*debug_buffer_ptr)++ = tokens->list[first_token + i].op;
  }
  *(*debug_buffer_ptr)++ = '\n';

  append_pointer(state->instr_ptr - first_token, 1, debug_buffer_ptr);
}

// Append the closest memory cells to the buffer
static void append_cells(Eval_state *state, size_t term_width,
                         char **debug_buffer_ptr) {
  const size_t cell_index = (size_t)(state->cur_cell - state->cells);
  const size_t half_row = term_width / SHOWN_CELL_WIDTH / 2;
  const size_t n_shown = term_width / SHOWN_CELL_WIDTH;
  const size_t first_cell = cell_index > half_row ? cell_index - half_row : 0;

  *debug_buffer_ptr += sprintf(*debug_buffer_ptr, "Cells:\n");

  for (size_t i = 0; i < n_shown; i++) {
    *debug_buffer_ptr +=
        sprintf(*debug_buffer_ptr, "%3zu  ", (first_cell + i) % 1000);
  }
  *(*debug_buffer_ptr)++ = '\n';

  for (size_t i = 0; i < n_shown; i++) {
    unsigned char cell_value = state->cells[first_cell + i];
    unsigned char chr = isprint(cell_value) ? cell_value : ' ';
    *debug_buffer_ptr += sprintf(*debug_buffer_ptr, "[ %c ]", chr);
  }
  *(*debug_buffer_ptr)++ = '\n';

  for (size_t i = 0; i < n_shown; i++) {
    *debug_buffer_ptr +=
        sprintf(*debug_buffer_ptr, "[%3d]", state->cells[first_cell + i]);
  }
  *(*debug_buffer_ptr)++ = '\n';

  append_pointer(cell_index, SHOWN_CELL_WIDTH, debug_buffer_ptr);
  *(*debug_buffer_ptr)++ = '\n';
}

// Append a visual pointer to the buffer
static void append_pointer(size_t steps, size_t step_size,
                           char **debug_buffer_ptr) {
  size_t n_chars = step_size * steps + step_size / 2;
  memset(*debug_buffer_ptr, ' ', n_chars);
  *(*debug_buffer_ptr + n_chars++) = '^';
  *(*debug_buffer_ptr + n_chars++) = '\n';
  *debug_buffer_ptr += n_chars;
}

// Clear the terminal
static void clear_terminal(void) { printf("%s", "\x1B[1;1H\x1B[2J"); }