CC = gcc
CFLAGS = -I. -I./emulator -I./modules -Wall -g

# source files
SOURCES = main.c emulator/byte_io.c emulator/emulator.c emulator/disasm.c modules/memory.c modules/interrupts.c

# header files
HEADERS = emulator/byte_io.h emulator/emulator.h emulator/disasm.h modules/memory.h modules/interrupts.h

OUTPUT = program

all: $(OUTPUT)

$(OUTPUT): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(OUTPUT)