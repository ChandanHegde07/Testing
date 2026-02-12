# SLM Context Window Manager 

PCC : Prompt Context Controller

A C-based system designed to efficiently manage conversation history within the limited context window constraints of modern Small Language Models (SLMs).

## Problem Statement

SLMs like GPT-3/4, Claude, and Llama have fixed context window sizes (typically 4k to 128k tokens) that limit the amount of conversation history they can process in a single request. When conversations exceed this limit, SLMs either reject the request or truncate the history arbitrarily, often losing critical context.

## Solution Overview

This system implements intelligent conversation history management using:

1. **Sliding Window Technique**: Maintains a contiguous window of messages within the token limit
2. **Priority-Based Message Retention**: Ensures important messages (system prompts, user questions) are retained
3. **Approximate Token Counting**: Fast heuristic-based token estimation without complex NLP libraries
4. **Dynamic Compression**: Removes or compresses old messages when token limits are exceeded
5. **Formatted Output**: Generates optimized context strings ready for SLM API consumption

## DSA Concepts Used

### 1. Linked List Data Structure
- **Doubly Linked List**: Efficient insertion/deletion operations at both ends
- **Dynamic Memory Management**: Allows flexible message storage
- **O(1) operations** for adding messages to the end and removing from the front

### 2. Sliding Window Algorithm
- Maintains a fixed-size window of messages within token limits
- New messages are added to the end
- Old messages are removed from the front when limits are exceeded

### 3. Priority Queue Implementation
- Messages assigned priority levels (CRITICAL, HIGH, NORMAL, LOW)
- Lower priority messages are removed first during compression
- Ensures critical information remains in context

### 4. Token Estimation Heuristics
- Simple heuristic: 1 token ≈ 4 characters (worst-case estimation)
- Fast calculation: `token_count = (str_length + 3) / 4`
- Balances accuracy and performance

### 5. Context Optimization
- Generates formatted context strings suitable for SLM APIs
- Proper message type labeling and formatting
- Handles edge cases like empty windows

## Project Structure

```
.
├── src/                # Core implementation
│   ├── context_window.c     # Context window manager implementation
│   └── main.c              # Example application
├── include/            # Header files
│   └── context_window.h    # Core functionality declarations
├── tests/              # Test suite
│   └── test_window_manager.c # Unit tests
├── examples/           # Example data
│   └── sample_conversation.txt # Sample conversation history
├── docs/               # Documentation
│   └── architecture.md       # Detailed architecture explanation
├── Makefile            # Build system
└── README.md           # This file
```

## Installation & Usage

### Prerequisites
- C compiler (GCC 4.0 or later)
- Standard C library
- POSIX-compliant system (Linux, macOS, Windows with WSL)

### Building the Project

```bash
# Compile the project
make all

# Run the test suite
make test

# Run the example program
make run

# Clean up compiled files
make clean
```

## Usage Example

```c
#include "context_window.h"

int main() {
    // Create context window with 1000 token limit
    ContextWindow* window = context_window_create(1000);
    
    // Add messages
    context_window_add_message(window, MESSAGE_SYSTEM, PRIORITY_CRITICAL, 
        "You are a helpful AI assistant.");
    
    context_window_add_message(window, MESSAGE_USER, PRIORITY_HIGH, 
        "Hello! Can you help me with C programming?");
    
    context_window_add_message(window, MESSAGE_ASSISTANT, PRIORITY_NORMAL, 
        "Of course! What do you need help with?");
    
    // Get optimized context
    char* context = context_window_get_context(window);
    printf("Context:\n%s", context);
    free(context);
    
    // Clean up
    context_window_destroy(window);
    
    return 0;
}
```

### Output
```
Context:
System: You are a helpful AI assistant.
User: Hello! Can you help me with C programming?
Assistant: Of course! What do you need help with?
```

## Example Input/Output

### Input
```
System: You are a helpful AI assistant.
User: Hello! Can you help me learn Python?
Assistant: Of course! I'd be happy to help you learn Python. What do you want to learn first?
User: I want to understand variables and data types.
Assistant: In Python, variables are used to store data. The main data types are integers, floats, strings, booleans, lists, and dictionaries.
User: That's very helpful! What about converting between data types?
Assistant: Python provides built-in functions to convert between data types. For example, int(), str(), float(), etc.
```

### Output with Window Size 500
```
System: You are a helpful AI assistant.
User: I want to understand variables and data types.
Assistant: In Python, variables are used to store data. The main data types are integers, floats, strings, booleans, lists, and dictionaries.
User: That's very helpful! What about converting between data types?
Assistant: Python provides built-in functions to convert between data types. For example, int(), str(), float(), etc.
```

## API Reference

### Core Functions

#### Context Window Management
- `ContextWindow* context_window_create(int max_tokens)` - Create new window
- `void context_window_destroy(ContextWindow* window)` - Destroy window and free memory
- `void context_window_print_stats(ContextWindow* window)` - Print window statistics

#### Message Handling
- `bool context_window_add_message(ContextWindow* window, MessageType type, MessagePriority priority, const char* content)` - Add new message
- `char* context_window_get_context(ContextWindow* window)` - Get formatted context string
- `int context_window_get_message_count(ContextWindow* window)` - Get message count
- `int context_window_get_token_count(ContextWindow* window)` - Get token count

#### Utility Functions
- `int calculate_token_count(const char* text)` - Estimate token count for string

### Enumerations

#### Message Types
- `MESSAGE_SYSTEM` - System prompt
- `MESSAGE_USER` - User message
- `MESSAGE_ASSISTANT` - Assistant message
- `MESSAGE_TOOL` - Tool response

#### Priority Levels
- `PRIORITY_CRITICAL` - Highest priority (system prompts)
- `PRIORITY_HIGH` - High priority (user questions)
- `PRIORITY_NORMAL` - Normal priority (assistant responses)
- `PRIORITY_LOW` - Low priority (detailed explanations, examples)

## Performance

### Time Complexity
- Adding a message: O(n) in worst case (due to compression), O(1) amortized
- Removing a message: O(1)
- Calculating context string: O(n)

### Space Complexity
- O(n) where n is the number of messages

### Memory Usage
- Each message typically uses ~100-200 bytes of overhead (plus content storage)
- Can handle thousands of messages per 1MB of memory

## Applications

### Direct API Integration
```c
// Prepare context for SLM API call
char* context = context_window_get_context(window);

// Example API request
send_slm_request(context, completion_callback);

// Free context after use
free(context);
```

### Web Application Backend
```c
// Handle user request
char* user_message = get_user_input();

// Add to context window
context_window_add_message(window, MESSAGE_USER, PRIORITY_HIGH, user_message);

// Generate optimized context for API
char* api_context = context_window_get_context(window);

// Send to SLM API and get response
char* ai_response = call_slm_api(api_context);

// Add response to window
context_window_add_message(window, MESSAGE_ASSISTANT, PRIORITY_NORMAL, ai_response);

// Return to client
send_response_to_client(ai_response);

// Clean up
free(api_context);
free(ai_response);
```

## Future Enhancements

1. **Intelligent Summarization**: Compress old messages by generating concise summaries
2. **Topic Detection**: Group related messages and retain key topics
3. **User Preferences**: Allow custom priority rules based on user-defined patterns
4. **Tokenization**: Integrate with real tokenizers like Hugging Face's tokenizers
5. **Configuration**: Support external configuration files for system parameters

## Contributing

Contributions are welcome! Please feel free to:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Submit a pull request

## License

MIT License - see LICENSE file for details.
