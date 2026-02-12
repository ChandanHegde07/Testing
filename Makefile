# SLM Context Window Manager - Makefile

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -g
LDFLAGS =

# Targets
TARGET = slm-context-manager
TEST_TARGET = test-window-manager

# Source files
SRC_DIR = src
INC_DIR = include
TEST_DIR = tests
EXAMPLES_DIR = examples
DOCS_DIR = docs

MAIN_SRC = $(SRC_DIR)/main.c
WINDOW_SRC = $(SRC_DIR)/context_window.c
TEST_SRC = $(TEST_DIR)/test_window_manager.c

# Object files
MAIN_OBJ = $(MAIN_SRC:.c=.o)
WINDOW_OBJ = $(WINDOW_SRC:.c=.o)
TEST_OBJ = $(TEST_SRC:.c=.o)

# Default target
all: $(TARGET) $(TEST_TARGET)

# Build the main application
$(TARGET): $(MAIN_OBJ) $(WINDOW_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Build the test suite
$(TEST_TARGET): $(TEST_OBJ) $(WINDOW_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Run the test suite
test: $(TEST_TARGET)
	@echo "Running test suite..."
	@./$(TEST_TARGET)

# Run the example program
run: $(TARGET)
	@echo "Running example program..."
	@./$(TARGET)

# Clean up compiled files
clean:
	@rm -f $(MAIN_OBJ) $(WINDOW_OBJ) $(TEST_OBJ)
	@rm -f $(TARGET) $(TEST_TARGET)
	@rm -f *.out

# Clean all temporary files and executables
distclean: clean
	@rm -rf $(TARGET) $(TEST_TARGET)
	@rm -rf *.dSYM  # For macOS debugging

# Show project structure
tree:
	@echo "Project Structure:"
	@tree -L 2 -I '*.o|*.dSYM' .

.PHONY: all clean distclean test run tree

# Dependencies
$(MAIN_OBJ): $(INC_DIR)/context_window.h
$(WINDOW_OBJ): $(INC_DIR)/context_window.h
$(TEST_OBJ): $(INC_DIR)/context_window.h
