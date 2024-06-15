CFLAGS = -Wall -Wextra -pedantic -O3 
tarpiter: src/tarpiter.c ; cc src/tarpiter.c -o tarpiter $(CFLAGS) 
