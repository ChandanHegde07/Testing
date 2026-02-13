# PCC - Prompt Context Controller

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: POSIX](https://img.shields.io/badge/Platform-POSIX-green.svg)](https://en.wikipedia.org/wiki/POSIX)

> A lightweight, efficient C-based system for managing conversation history within the limited context window constraints of Small Language Models (SLMs).

---

## Table of Contents

- [Overview](#overview)
- [The Problem](#the-problem)
- [Features](#features)
- [How It Works](#how-it-works)
- [Project Structure](#project-structure)
- [Quick Start](#quick-start)
- [Installation](#installation)
- [Usage](#usage)
- [API Reference](#api-reference)
- [Performance](#performance)
- [Documentation](#documentation)
- [Examples](#examples)
- [Roadmap](#roadmap)
- [Contributing](#contributing)
- [License](#license)

---

## Overview

**PCC (Prompt Context Controller)** is a C library designed to efficiently manage conversation history for Small Language Models. It implements intelligent context window management using proven data structures and algorithms, ensuring optimal token utilization while preserving critical conversation context.

### Why PCC?

| Feature | Description |
|---------|-------------|
| **Lightweight** | Pure C implementation with minimal dependencies |
| **Fast** | O(1) amortized message insertion with efficient memory management |
| **Smart Retention** | Priority-based message retention keeps important context |
| **Token Aware** | Approximate token counting without heavy NLP libraries |
| **Easy Integration** | Simple API for direct integration into existing projects |

---

## The Problem

Small Language Models (SLMs) like Phi-2, TinyLlama, and Gemma have limited context windows (typically 2K-8K tokens). When conversations exceed these limits:

- API requests get rejected
- Critical context gets arbitrarily truncated
- Important system prompts may be lost
- Conversation coherence degrades

**PCC solves this** by intelligently managing the context window, ensuring the most relevant information stays within token limits.

---

## Features

- **Sliding Window Technique** - Maintains contiguous message window within token limits
- **Priority-Based Retention** - Critical messages (system prompts) are preserved
- **Fast Token Estimation** - Heuristic-based counting (~4 chars per token)
- **Dynamic Compression** - Automatic removal of low-priority messages
- **Formatted Output** - Ready-to-use context strings for SLM APIs
- **Memory Safe** - Proper allocation/deallocation with leak prevention

---

## How It Works

### Data Structures

```
┌─────────────────────────────────────────────────────────────┐
│                    ContextWindow                            │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ max_tokens: 1000                                     │   │
│  │ total_tokens: 847                                    │   │
│  │ message_count: 5                                     │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│  head ──► [System] ──► [User] ──► [Asst] ──► [User] ──► ◄── tail
│            CRITICAL     HIGH       NORMAL      HIGH        │
│            (kept)      (kept)     (removable)  (kept)      │
└─────────────────────────────────────────────────────────────┘
```

### Algorithm Flow

```
New Message Arrives
        │
        ▼
┌───────────────────┐
│ Calculate Tokens  │
└─────────┬─────────┘
          │
          ▼
┌───────────────────┐     ┌────────────────────┐
│ Add to Window     │────►│ Exceeds Limit?     │
└───────────────────┘     └─────────┬──────────┘
                                    │
                          ┌─────────┴─────────┐
                          │ Yes               │ No
                          ▼                   ▼
                ┌─────────────────┐    ┌──────────────┐
                │ Remove Lowest   │    │   Done       │
                │ Priority Msgs   │    └──────────────┘
                └─────────────────┘
```

---

## Project Structure

```
PCC/
├── src/
│   ├── context_window.c    # Core implementation
│   └── main.c              # Demo application
├── include/
│   └── context_window.h    # Public API header
├── tests/
│   └── test_window_manager.c  # Unit tests
├── examples/
│   └── sample_conversation.txt # Sample data
├── docs/
│   ├── architecture.md     # System architecture
│   └── design.md           # Design decisions
├── Makefile                # Build system
├── LICENSE                 # MIT License
└── Readme.md               # This file
```

---

## Quick Start

```bash
# Clone the repository
git clone https://github.com/ChandanHegde07/PCC.git
cd pcc

# Build and run
make all && make run
```

**Expected Output:**
```
SLM Context Window Manager
==========================

Adding initial messages...

Context Window Statistics:
--------------------------
Max Tokens:     1000
Current Tokens: 847
Message Count:  6
Utilization:    84.7%

Optimized Context for SLM API:
-------------------------------
System: You are a helpful AI assistant...
User: Hello! Can you help me with a programming problem?
Assistant: Of course! What do you need help with?
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

# Clean build artifacts
make clean

# Complete clean (includes dist files)
make distclean
```

---

## Usage

### Basic Example

```c
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
| `context_window_destroy(ContextWindow* window)` | Free all resources | `void` |
| `context_window_add_message(...)` | Add message to window | `bool` |
| `context_window_get_context(...)` | Get formatted context string | `char*` |
| `context_window_get_message_count(...)` | Get number of messages | `int` |
| `context_window_get_token_count(...)` | Get total token count | `int` |
| `context_window_print_stats(...)` | Print window statistics | `void` |
| `calculate_token_count(const char* text)` | Estimate tokens in string | `int` |

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

## Documentation

Additional documentation is available in the [`docs/`](docs/) directory:

- **[Architecture Guide](docs/architecture.md)** - Detailed system architecture and component overview
- **[Design Document](docs/design.md)** - Design decisions and trade-offs

---

## Examples

### Sample Conversation Processing

**Input** (from [`examples/sample_conversation.txt`](examples/sample_conversation.txt)):
```
System: You are a helpful AI assistant.
User: Hello! Can you help me learn Python?
Assistant: Of course! I'd be happy to help...
User: I want to understand variables and data types.
Assistant: In Python, variables are used to store data...
```

**Output** (with 500 token window):
```
System: You are a helpful AI assistant.
User: I want to understand variables and data types.
Assistant: In Python, variables are used to store data...
```

*Note: Earlier messages were automatically removed to fit within token limits.*

---

## Roadmap

### Planned Features

- [ ] **Intelligent Summarization** - Compress old messages into concise summaries
- [ ] **Topic Detection** - Group related messages by topic
- [ ] **Custom Priority Rules** - User-defined patterns for priority assignment
- [ ] **Real Tokenization** - Integration with HuggingFace tokenizers
- [ ] **Configuration Files** - External config for system parameters
- [ ] **Thread Safety** - Mutex support for multi-threaded applications
- [ ] **Persistence** - Save/load conversation state to disk

---

## Contributing

Contributions are welcome! Here's how to get started:

1. **Fork** the repository
2. **Create** a feature branch (`git checkout -b feature/amazing-feature`)
3. **Commit** your changes (`git commit -m 'Add amazing feature'`)
4. **Push** to the branch (`git push origin feature/amazing-feature`)
5. **Open** a Pull Request

### Development Guidelines

- Follow the existing code style
- Add tests for new functionality
- Update documentation as needed
- Ensure all tests pass (`make test`)

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

```
MIT License

Copyright (c) 2024 PCC Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software...
```

---

<p align="center">
  <strong>Built for the SLM community</strong>
</p>

<p align="center">
  <a href="#overview">Back to Top</a>
</p>
