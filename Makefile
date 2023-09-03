# Compiler
CC = gcc

# Compiler flags
CFLAGS = -std=c99 -Wall -Wextra -pedantic -I/usr/include/SDL2

# Linker flags
LDFLAGS = -lSDL2

# Source files
SOURCES = game_of_life.c

# Executable name
EXECUTABLE = game_of_life

# Default target
all: $(EXECUTABLE)

# Compile and link the program
$(EXECUTABLE): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(EXECUTABLE) $(LDFLAGS)

# Clean up intermediate and executable files
clean:
	rm -f $(EXECUTABLE)

# Run the program
run: $(EXECUTABLE)
	./$(EXECUTABLE)

