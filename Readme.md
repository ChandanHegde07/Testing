# PCC - Prompt Context Controller

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: POSIX](https://img.shields.io/badge/Platform-POSIX-green.svg)](https://en.wikipedia.org/wiki/POSIX)
[![Build](https://github.com/ChandanHegde07/PCC/actions/workflows/ci.yml/badge.svg)](https://github.com/ChandanHegde07/PCC/actions)
[![Tests](https://img.shields.io/badge/Tests-23%20passed-green.svg)](https://github.com/ChandanHegde07/PCC/actions)

> A lightweight, efficient C-based system for managing conversation history within the limited context window constraints of Small Language Models (SLMs).

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Quick Start](#quick-start)
- [Project Structure](#project-structure)
- [Installation](#installation)
- [Usage](#usage)
- [API Reference](#api-reference)
- [Configuration](#configuration)
- [Save/Load](#saveload)
- [Performance](#performance)
- [Testing](#testing)
- [CI/CD](#cicd)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [License](#license)

---

## Overview

**PCC (Prompt Context Controller)** is a production-ready C library designed to efficiently manage conversation history for Small Language Models (SLMs). It implements intelligent context window management using proven data structures and algorithms, ensuring optimal token utilization while preserving critical conversation context.

### Why PCC?

| Feature | Description |
|---------|-------------|
| **Lightweight** | Pure C implementation with minimal dependencies |
| **Fast** | O(1) amortized message insertion with efficient memory management |
| **Smart Retention** | Priority-based message retention keeps important context |
| **Token Aware** | Approximate token counting without heavy NLP libraries |
| **Easy Integration** | Simple API for direct integration into existing projects |
| **Production Ready** | Comprehensive tests, CI/CD, and documentation |
| **Metrics** | Built-in metrics tracking for monitoring |
| **Configurable** | Flexible configuration system for custom behavior |
| **Persistent** | Save/load conversation state |

---

## Features

- **Sliding Window Technique** - Maintains contiguous message window within token limits
- **Priority-Based Retention** - Critical messages (system prompts) are preserved
- **Fast Token Estimation** - Heuristic-based counting (~4 chars per token)
- **Dynamic Compression** - Automatic removal of low-priority messages
- **Formatted Output** - Ready-to-use context strings for SLM APIs
- **Memory Safe** - Proper allocation/deallocation with leak prevention
- **Metrics Tracking** - Track evictions, utilization, and performance
- **Configuration System** - Customizable compression and behavior
- **Save/Load** - Persist context to file (text and JSON)
- **CI/CD** - Automated build, test, and deployment
- **Comprehensive Tests** - 23+ test cases with edge cases

---

## Project Structure

```
PCC/
├── .github/
│   └── workflows/
│       └── ci.yml              # GitHub Actions CI/CD
├── src/
│   ├── context_window.c        # Core implementation
│   └── main.c                  # Demo application
├── include/
│   └── context_window.h        # Public API header
├── tests/
│   ├── test_window_manager.c   # Comprehensive test suite
│   └── benchmark.c             # Performance benchmarks
├── examples/
│   ├── basic_usage.c           # Basic usage example
│   ├── config_example.c        # Configuration example
│   ├── save_load_example.c     # Save/load example
│   └── sample_conversation.txt # Sample data
├── docs/
│   ├── architecture.md         # System architecture
│   └── design.md               # Design decisions
├── Doxyfile                   # Doxygen configuration
├── Makefile                   # Build system
├── LICENSE                    # MIT License
└── Readme.md                  # This file
```

---

## Quick Start

```bash
# Clone the repository
git clone https://github.com/ChandanHegde07/PCC.git
cd PCC

# Build everything
make all

# Run the demo
make run

# Run tests
make test
```

**Expected Output:**
```
==============================================
  PCC - Prompt Context Controller
  Version 1.0.0
==============================================
...
Context Window Statistics:
  Total messages: 6
  Total tokens: 156/1000 (15.6% full)
  Tokens remaining: 844
...
```

---

## Installation

### Prerequisites

| Requirement | Version | Notes |
|-------------|---------|-------|
| GCC | 4.0+ | Or any C99-compliant compiler |
| Make | Any | For build automation |
| OS | POSIX | Linux, macOS, or WSL on Windows |

### Build Commands

```bash
# Build everything (main app + tests)
make all

# Run the demo application
make run

# Run the test suite
make test

# Run with valgrind (memory leak check)
make memtest

# Run with AddressSanitizer
make asan

# Run performance benchmarks
make benchmark

# Clean build artifacts
make clean
```

---

## Usage

### Basic Example

```c
#include <stdio.h>
#include "context_window.h"

int main(void) {
    // Create a context window with 1000 token limit
    ContextWindow* window = context_window_create(1000);
    
    // Add system prompt (highest priority - always retained)
    context_window_add_message(window, MESSAGE_SYSTEM, PRIORITY_CRITICAL, 
        "You are a helpful AI assistant.");
    
    // Add conversation messages
    context_window_add_message(window, MESSAGE_USER, PRIORITY_HIGH, 
        "What is the capital of France?");
    
    context_window_add_message(window, MESSAGE_ASSISTANT, PRIORITY_NORMAL, 
        "The capital of France is Paris.");
    
    // Get optimized context for SLM API
    char* context = context_window_get_context(window);
    printf("Context:\n%s\n", context);
    
    // Print statistics
    context_window_print_stats(window);
    
    // Cleanup
    free(context);
    context_window_destroy(window);
    
    return 0;
}
```

### Integration with SLM APIs

```c
// Typical workflow for SLM integration
void process_user_query(ContextWindow* window, const char* user_input) {
    // 1. Add user message
    context_window_add_message(window, MESSAGE_USER, PRIORITY_HIGH, user_input);
    
    // 2. Get optimized context
    char* context = context_window_get_context(window);
    
    // 3. Send to your SLM API
    char* response = call_slm_api(context);  // Your API call
    
    // 4. Store assistant response
    context_window_add_message(window, MESSAGE_ASSISTANT, PRIORITY_NORMAL, response);
    
    // 5. Cleanup
    free(context);
    free(response);
}
```

---

## API Reference

### Core Functions

| Function | Description | Returns |
|----------|-------------|---------|
| `context_window_create(int max_tokens)` | Create new context window | `ContextWindow*` |
| `context_window_create_with_config(const ContextConfig*)` | Create with custom config | `ContextWindow*` |
| `context_window_destroy(ContextWindow*)` | Free all resources | `void` |
| `context_window_add_message(...)` | Add message to window | `bool` |
| `context_window_add_message_ex(...)` | Add message with result code | `bool` |
| `context_window_remove_message(...)` | Remove specific message | `bool` |
| `context_window_clear(ContextWindow*)` | Clear all messages | `void` |
| `context_window_get_context(ContextWindow*)` | Get formatted context string | `char*` |
| `context_window_get_context_json(ContextWindow*)` | Get context as JSON | `char*` |
| `context_window_get_message_count(...)` | Get number of messages | `int` |
| `context_window_get_token_count(...)` | Get total token count | `int` |
| `context_window_get_utilization(...)` | Get utilization % | `double` |
| `context_window_print_stats(ContextWindow*)` | Print window statistics | `void` |
| `context_window_print_metrics(ContextWindow*)` | Print detailed metrics | `void` |
| `calculate_token_count(const char*)` | Estimate tokens in string | `int` |

### Save/Load Functions

| Function | Description | Returns |
|----------|-------------|---------|
| `context_window_save(...)` | Save to text file | `CWResult` |
| `context_window_load(...)` | Load from text file | `ContextWindow*` |
| `context_window_export_json(...)` | Export to JSON file | `CWResult` |

### Configuration Functions

| Function | Description | Returns |
|----------|-------------|---------|
| `context_config_default()` | Get default config | `ContextConfig` |
| `context_config_validate(...)` | Validate config | `bool` |
| `context_window_apply_config(...)` | Apply new config | `CWResult` |

### Enumerations

#### MessageType

```c
typedef enum {
    MESSAGE_USER,      // User input
    MESSAGE_ASSISTANT, // AI response
    MESSAGE_SYSTEM,    // System prompt
    MESSAGE_TOOL       // Tool/function response
} MessageType;
```

#### MessagePriority

```c
typedef enum {
    PRIORITY_LOW,      // Can be removed first
    PRIORITY_NORMAL,   // Default priority
    PRIORITY_HIGH,     // User questions
    PRIORITY_CRITICAL  // System prompts (never removed)
} MessagePriority;
```

#### CompressionStrategy

```c
typedef enum {
    COMPRESSION_NONE,      // No compression
    COMPRESSION_LOW_PRIORITY, // Remove low priority first
    COMPRESSION_AGGRESSIVE   // Aggressive compression
} CompressionStrategy;
```

---

## Configuration

### Using Custom Configuration

```c
#include "context_window.h"

int main(void) {
    // Get default configuration
    ContextConfig config = context_config_default();
    
    // Customize settings
    config.max_tokens = 2000;
    config.compression = COMPRESSION_LOW_PRIORITY;
    config.enable_metrics = true;
    config.auto_compress = true;
    
    // Create window with custom config
    ContextWindow* window = context_window_create_with_config(&config);
    
    // Use window...
    
    context_window_destroy(window);
    return 0;
}
```

---

## Save/Load

### Save to File

```c
// Save context to text file
CWResult result = context_window_save(window, "conversation.txt");

// Export to JSON
CWResult result = context_window_export_json(window, "conversation.json");
```

### Load from File

```c
// Load context from file
ContextWindow* window = context_window_load("conversation.txt");

// Use loaded context...

context_window_destroy(window);
```

---

## Performance

### Time Complexity

| Operation | Complexity | Notes |
|-----------|------------|-------|
| Add message | O(1) amortized | O(n) worst case during compression |
| Remove message | O(1) | Direct pointer manipulation |
| Get context | O(n) | Must traverse all messages |
| Token count | O(1) | Stored and updated incrementally |

### Space Complexity

- **Overall**: O(n) where n = number of messages
- **Per message**: ~100-200 bytes overhead + content length

### Memory Efficiency

| Messages | Approximate Memory |
|----------|-------------------|
| 100 | ~50 KB |
| 1,000 | ~500 KB |
| 10,000 | ~5 MB |

---

## Testing

```bash
# Run test suite
make test

# Run with valgrind (memory leak check)
make memtest

# Run with AddressSanitizer
make asan
```

### Test Coverage

- Basic functionality (create, destroy, add, remove)
- Edge cases (NULL inputs, empty window, boundary values)
- Priority-based eviction
- Token calculation
- Memory stress tests
- Save/load operations
- Configuration validation

---

## CI/CD

The project uses GitHub Actions for continuous integration and deployment:

- **Build**: Compiles with `-Wall -Wextra -Wpedantic -Werror` for strict compilation
- **Test**: Runs comprehensive test suite with detailed reporting
- **Benchmark**: Measures performance metrics
- **Static Analysis**: Checks code quality with cppcheck
- **Artifacts**: Uploads binaries for download and versioning

---

## Documentation

Generate API documentation with Doxygen:

```bash
make docs
```

Additional documentation in [`docs/`](docs/):
- **[Architecture Guide](docs/architecture.md)** - Detailed system architecture
- **[Design Document](docs/design.md)** - Design decisions and trade-offs

---

## Contributing

Contributions are welcome!

1. **Fork** the repository
2. **Create** a feature branch
3. **Commit** your changes
4. **Push** to the branch
5. **Open** a Pull Request

### Development Guidelines

- Follow the existing code style
- Add tests for new functionality
- Ensure all tests pass (`make test`)
- Build with zero warnings

---

## License

MIT License - see [LICENSE](LICENSE) file for details.

---

<p align="center">
  <strong>Built for the SLM community</strong>
</p>

<p align="center">
  <a href="#overview">Back to Top</a>
</p>
