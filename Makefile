# PCC - Prompt Context Controller Makefile
# Production-ready build configuration

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Werror -std=c99 -O2 -Iinclude
DEBUG_CFLAGS = -Wall -Wextra -Wpedantic -Werror -std=c99 -g -Iinclude -fsanitize=address
LDFLAGS = -lm
VALGRIND = valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes

# Project info
PROJECT_NAME = PCC
VERSION = 1.0.0

# Targets
TARGET = slm-context-manager
TEST_TARGET = test-window-manager
BENCHMARK_TARGET = benchmark

# Directories
SRC_DIR = src
INC_DIR = include
TEST_DIR = tests
EXAMPLES_DIR = examples
DOCS_DIR = docs

# Source files
MAIN_SRC = $(SRC_DIR)/main.c
WINDOW_SRC = $(SRC_DIR)/context_window.c
TEST_SRC = $(TEST_DIR)/test_window_manager.c
BENCHMARK_SRC = $(TEST_DIR)/benchmark.c

# Object files
MAIN_OBJ = $(SRC_DIR)/main.o
WINDOW_OBJ = $(SRC_DIR)/context_window.o
TEST_OBJ = $(TEST_DIR)/test_window_manager.o
BENCHMARK_OBJ = $(TEST_DIR)/benchmark.o

# Documentation files
DOCS = Readme.md docs/architecture.md docs/design.md
API_DOCS = docs/api.md

# Default target - build everything
.PHONY: all
all: info $(TARGET) $(TEST_TARGET)
	@echo ""
	@echo "Build complete!"
	@echo "Run 'make test' to execute tests"
	@echo "Run 'make run' to run the demo"

# Info target
.PHONY: info
info:
	@echo "=============================================="
	@echo "  $(PROJECT_NAME) - Prompt Context Controller"
	@echo "  Version $(VERSION)"
	@echo "=============================================="

# Build the main application
$(TARGET): $(MAIN_OBJ) $(WINDOW_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Built: $(TARGET)"

# Build the test suite
$(TEST_TARGET): $(TEST_OBJ) $(WINDOW_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Built: $(TEST_TARGET)"

# Build the benchmark
$(BENCHMARK_TARGET): $(BENCHMARK_OBJ) $(WINDOW_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Built: $(BENCHMARK_TARGET)"

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Run the test suite
.PHONY: test
test: $(TEST_TARGET)
	@echo ""
	@echo "Running test suite..."
	@echo "=============================================="
	@./$(TEST_TARGET)

# Run with valgrind (memory leak check)
.PHONY: memtest
memtest: $(TEST_TARGET)
	@echo ""
	@echo "Running memory leak check with valgrind..."
	@echo "=============================================="
	$(VALGRIND) ./$(TEST_TARGET)

# Run with address sanitizer
.PHONY: asan
asan: DEBUG_CFLAGS += -fsanitize=address
asan: clean
	$(CC) $(DEBUG_CFLAGS) -o $(TEST_TARGET) $(TEST_SRC) $(WINDOW_SRC) $(LDFLAGS)
	@echo ""
	@echo "Running with AddressSanitizer..."
	@echo "=============================================="
	./$(TEST_TARGET)

# Run the example program
.PHONY: run
run: $(TARGET)
	@echo ""
	@echo "Running example program..."
	@echo "=============================================="
	@./$(TARGET)

# Run performance benchmarks
.PHONY: benchmark
benchmark: $(BENCHMARK_TARGET)

# Build with debug symbols
.PHONY: debug
debug: DEBUG_CFLAGS = -Wall -Wextra -g -Iinclude -DDEBUG
debug: clean
	$(CC) $(DEBUG_CFLAGS) -o $(TARGET) $(MAIN_SRC) $(WINDOW_SRC) $(LDFLAGS)
	$(CC) $(DEBUG_CFLAGS) -o $(TEST_TARGET) $(TEST_SRC) $(WINDOW_SRC) $(LDFLAGS)
	@echo "Debug build complete"

# Clean build artifacts
.PHONY: clean
clean:
	@rm -f $(SRC_DIR)/*.o $(TEST_DIR)/*.o
	@rm -f $(TARGET) $(TEST_TARGET) $(BENCHMARK_TARGET)
	@rm -f *.out

# Complete clean (includes dist files)
.PHONY: distclean
distclean: clean
	@rm -rf *.dSYM  # For macOS debugging
	@rm -rf docs/html  # Doxygen output

# Install (POSIX systems)
.PHONY: install
install: $(TARGET)
	@mkdir -p $(DESTDIR)/usr/local/bin
	@mkdir -p $(DESTDIR)/usr/local/include
	@cp -n $(TARGET) $(DESTDIR)/usr/local/bin/
	@cp -n $(INC_DIR)/*.h $(DESTDIR)/usr/local/include/
	@echo "Installed PCC to $(DESTDIR)/usr/local/"

# Uninstall
.PHONY: uninstall
uninstall:
	@rm -f $(DESTDIR)/usr/local/bin/$(TARGET)
	@rm -f $(DESTDIR)/usr/local/include/context_window.h
	@echo "Uninstalled PCC"

# Generate Doxygen documentation
.PHONY: docs
docs:
	@command -v doxygen >/dev/null 2>&1 && doxygen Doxyfile || echo "Doxygen not installed. Install doxygen to generate API docs."

# Show project structure
.PHONY: tree
	@echo "Project Structure:"
	@find . -type f -name "*.c" -o -name "*.h" -o -name "Makefile" -o -name "*.md" | grep -v ".git" | sort

# Static analysis with cppcheck
.PHONY: analyze
analyze:
	@command -v cppcheck >/dev/null 2>&1 && \
		cppcheck --enable=all --std=c99 --platform=unix64 --quiet src/ include/ || \
		echo "cppcheck not installed. Install cppcheck for static analysis."

# Format code (requires clang-format)
.PHONY: format
format:
	@command -v clang-format >/dev/null 2>&1 && \
		find src include tests -name "*.c" -o -name "*.h" | xargs clang-format -i || \
		echo "clang-format not installed"

# Check code style
.PHONY: style
style:
	@echo "Checking code style..."
	@grep -rn "TODO\|FIXME\|XXX\|HACK" src/ include/ tests/ || echo "No TODO/FIXME comments found"

# Dependencies
$(MAIN_OBJ): $(INC_DIR)/context_window.h
$(WINDOW_OBJ): $(INC_DIR)/context_window.h
$(TEST_OBJ): $(INC_DIR)/context_window.h
$(BENCHMARK_OBJ): $(INC_DIR)/context_window.h

# Help target
.PHONY: help
help:
	@echo "PCC Build System"
	@echo "================"
	@echo ""
	@echo "Targets:"
	@echo "  all         - Build everything (default)"
	@echo "  test        - Run the test suite"
	@echo "  memtest     - Run tests with valgrind"
	@echo "  asan        - Run tests with AddressSanitizer"
	@echo "  run         - Run the demo application"
	@echo "  benchmark   - Run performance benchmarks"
	@echo "  debug       - Build with debug symbols"
	@echo "  clean       - Remove build artifacts"
	@echo "  distclean   - Complete clean"
	@echo "  install     - Install to system"
	@echo "  uninstall   - Remove from system"
	@echo "  docs        - Generate Doxygen documentation"
	@echo "  analyze     - Run static analysis"
	@echo "  format      - Format code with clang-format"
	@echo "  style       - Check code style"
	@echo "  help        - Show this help"
	@echo ""
	@echo "Examples:"
	@echo "  make                # Build everything"
	@echo "  make test          # Run tests"
	@echo "  make benchmark     # Run benchmarks"
	@echo "  make memtest       # Check for memory leaks"
