CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -g -Isrc
TARGET  = vm-x86

SRCS    = src/main.c src/cpu.c src/memory.c src/decode.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(TARGET)
	./$(TARGET) --test

clean:
	rm -f $(OBJS) $(TARGET)
