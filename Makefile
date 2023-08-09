# Compiler flags
CC = gcc
CFLAGS = -Wall -Iinclude $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs)

# Source files
MAIN_SRCS = $(wildcard src/emulator/*.c src/interface/*.c src/utils/*.c src/main.c)
TEST_SRCS = $(wildcard src/emulator/memory.c src/emulator/processor.c src/utils/disasm.c tests/tests.c)

# Executable names
MAIN_EXEC = i8080-invaders
TEST_EXEC = cpu-test

all: clean $(MAIN_EXEC)

$(MAIN_EXEC):
	$(CC) $(CFLAGS) -o $@ $(MAIN_SRCS) $(LDFLAGS)

test: clean $(TEST_EXEC)

$(TEST_EXEC):
	$(CC) $(CFLAGS) -o $@ $(TEST_SRCS) -DFOR_CPUDIAG

clean:
	rm -f $(MAIN_EXEC) $(TEST_EXEC)