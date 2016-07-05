CC = x86_64-w64-mingw32-gcc
CFLAGS = -Werror -Wall -Wextra -Wconversion -std=c99 -O2
BINARY = wav2agb
LIBS = -lsndfile -lm

SRC_FILES = $(wildcard *.c)
OBJ_FILES = $(SRC_FILES:.c=.o)


all: $(BINARY)
	

.PHONY: clean
clean:
	rm -f $(OBJ_FILES)

$(BINARY): $(OBJ_FILES)
	$(CC) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(IMPORT)
