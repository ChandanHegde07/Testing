#ifndef CONTEXT_WINDOW_H
#define CONTEXT_WINDOW_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Message types
typedef enum {
    MESSAGE_USER,
    MESSAGE_ASSISTANT,
    MESSAGE_SYSTEM,
    MESSAGE_TOOL
} MessageType;

// Message importance levels
typedef enum {
    PRIORITY_LOW,
    PRIORITY_NORMAL,
    PRIORITY_HIGH,
    PRIORITY_CRITICAL
} MessagePriority;

// Struct to represent a single conversation message
typedef struct Message {
    MessageType type;              // Type of message (user, assistant, etc.)
    MessagePriority priority;      // Importance level
    char* content;                 // Message content
    int token_count;               // Approximate token count
    struct Message* next;          // Next message in linked list
    struct Message* prev;          // Previous message in linked list
} Message;

// Struct to represent the context window manager
typedef struct ContextWindow {
    Message* head;                 // Head of the message queue
    Message* tail;                 // Tail of the message queue
    int total_tokens;              // Total tokens in current window
    int max_tokens;                // Maximum tokens allowed in context window
    int message_count;             // Number of messages in window
} ContextWindow;

// Initialize a new context window with specified token limit
ContextWindow* context_window_create(int max_tokens);

// Destroy a context window and free all memory
void context_window_destroy(ContextWindow* window);

// Add a new message to the context window
// Returns true if message was added successfully, false if rejected
bool context_window_add_message(ContextWindow* window,
                                MessageType type,
                                MessagePriority priority,
                                const char* content);

// Get the optimized context text ready for SLM API
// Returns a dynamically allocated string that must be freed
char* context_window_get_context(ContextWindow* window);

// Get total number of messages in context window
int context_window_get_message_count(ContextWindow* window);

// Get total token count in context window
int context_window_get_token_count(ContextWindow* window);

// Calculate approximate token count for a string
int calculate_token_count(const char* text);

// Print context window statistics
void context_window_print_stats(ContextWindow* window);

#endif // CONTEXT_WINDOW_H
