CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = wordsearch
SRC = wordsearch.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

.PHONY: all clean
