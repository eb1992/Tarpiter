CFLAGS = -Wall -Wextra -pedantic -O3 
BIN_DIR = $(HOME)/.local/bin/

tarpiter: src/tarpiter.c
	mkdir -p $(BIN_DIR)
	cc src/tarpiter.c -o tarpiter $(CFLAGS) 
	mv tarpiter $(BIN_DIR)