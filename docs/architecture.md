# SLM Context Window Manager - Architecture

## Overview

The SLM Context Window Manager is a C-based system designed to efficiently manage conversation history within the limited context window constraints of modern Small Language Models (SLMs). It implements intelligent message retention and compression strategies to ensure that the most important information remains within the context window while staying under token limits.

## Problem Statement

SLMs like GPT-3/4, Claude, and Llama have fixed context window sizes that limit the amount of conversation history they can process in a single request. This creates several challenges:

1. **Token Limit Exceedance**: Conversations longer than the token limit are rejected
2. **Information Loss**: Truncating conversations arbitrarily can lose important context
3. **Inefficient Context Management**: Manual management of conversation history is time-consuming and error-prone
4. **Priority Handling**: Not all messages are equally important; some contain critical information

## Solution Architecture

The system is designed around three core architectural principles:

### 1. Data Structures

#### Linked List Implementation
- Doubly linked list for efficient message management
- Each node represents a conversation message with metadata
- Allows O(1) insertion at both ends and O(n) traversal

```c
typedef struct Message {
    MessageType type;              // Type of message
    MessagePriority priority;      // Importance level
    char* content;                 // Message content
    int token_count;               // Token estimation
    struct Message* next;          // Next node pointer
    struct Message* prev;          // Previous node pointer
} Message;
```

#### Context Window Structure
```c
typedef struct ContextWindow {
    Message* head;                 // Head of the message queue
    Message* tail;                 // Tail of the message queue
    int total_tokens;              // Total tokens in window
    int max_tokens;                // Token limit
    int message_count;             // Number of messages
} ContextWindow;
```

### 2. Core Algorithms

#### Sliding Window Technique
- Maintains a contiguous window of messages within the token limit
- New messages are added to the end (tail) of the window
- Old messages are removed from the beginning (head) when limits are exceeded

#### Priority-Based Message Retention
- Messages are assigned priority levels: PRIORITY_LOW, PRIORITY_NORMAL, PRIORITY_HIGH, PRIORITY_CRITICAL
- When compression is needed, lower priority messages are removed first
- Critical messages are retained as long as possible

#### Token Estimation
- Simple heuristic: 1 token ≈ 4 characters (worst-case estimation)
- Allows quick calculation of token counts without complex NLP libraries
- Estimation formula: `token_count = (str_length + 3) / 4`

### 3. System Components

#### Message Management
- `context_window_add_message()` - Adds new messages with type and priority
- `remove_message()` - Removes specific messages from the linked list
- `context_window_get_context()` - Generates formatted context string for SLM API

#### Compression Strategy
1. First, remove all PRIORITY_LOW messages
2. If still over limit, remove PRIORITY_NORMAL messages
3. If still over limit, remove PRIORITY_HIGH messages
4. As a last resort, remove PRIORITY_CRITICAL messages (should rarely happen)

#### Statistics and Monitoring
- `context_window_get_token_count()` - Returns current token usage
- `context_window_get_message_count()` - Returns number of messages
- `context_window_print_stats()` - Prints comprehensive window statistics

## Design Patterns

### 1. Queue Pattern
- FIFO (First-In-First-Out) behavior for normal message handling
- Supports sliding window operations

### 2. Priority Queue Pattern
- Messages are retained based on priority level
- Lower priority messages are discarded first

### 3. Iterator Pattern
- Linked list traversal for message processing
- Allows for flexible message compression algorithms

### 4. Factory Pattern (Message Creation)
- `create_message()` function encapsulates message construction
- Handles memory allocation and initialization

## Memory Management

- All messages are dynamically allocated and properly freed
- `context_window_destroy()` cleans up all resources
- Each message's content is duplicated using `strdup()` for safety
- Error handling for memory allocation failures

## Performance Characteristics

- **Time Complexity**:
  - Adding a message: O(n) in worst case (due to compression), O(1) amortized
  - Removing a message: O(1)
  - Calculating context string: O(n)
- **Space Complexity**: O(n) where n is the number of messages
- **Compression Speed**: Fast heuristic-based compression ensures low latency

## Extensibility

The architecture is designed to be easily extensible:

1. **Token Estimation**: Can be replaced with more accurate NLP-based tokenizers
2. **Compression Algorithms**: Priority-based strategy can be extended with ML-based approaches
3. **Message Types**: New message types can be added by extending the MessageType enum
4. **Priority Levels**: Additional priority levels can be added by extending MessagePriority

## Future Enhancements

1. **Intelligent Summarization**: Compress old messages by generating concise summaries
2. **Topic Detection**: Group related messages and retain key topics
3. **User Preferences**: Allow custom priority rules based on user-defined patterns
4. **Tokenization**: Integrate with real tokenizers like Hugging Face's tokenizers
5. **Configuration**: Support external configuration files for system parameters

## System Requirements

- C compiler (GCC 4.0 or later)
- Standard C library
- POSIX-compliant system (Linux, macOS, Windows with WSL)

## Build System

Simple Makefile for compiling and testing:
- `make all` - Build library and test suite
- `make test` - Run all tests
- `make clean` - Remove compiled files
