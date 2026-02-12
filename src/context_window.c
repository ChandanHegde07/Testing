// SLM Context Window Manager
// Implementation of core functionality

#include "context_window.h"

// Simple token estimation: 1 token ~= 4 characters (rough approximation)
#define TOKEN_ESTIMATION_RATIO 4

// Calculate approximate token count for a string
int calculate_token_count(const char* text) {
    if (text == NULL) return 0;
    
    int length = strlen(text);
    return (length + TOKEN_ESTIMATION_RATIO - 1) / TOKEN_ESTIMATION_RATIO;
}

// Initialize a new context window with specified token limit
ContextWindow* context_window_create(int max_tokens) {
    ContextWindow* window = (ContextWindow*)malloc(sizeof(ContextWindow));
    if (window == NULL) {
        printf("Failed to allocate memory for context window\n");
        return NULL;
    }
    
    window->head = NULL;
    window->tail = NULL;
    window->total_tokens = 0;
    window->max_tokens = max_tokens;
    window->message_count = 0;
    
    return window;
}

// Destroy a context window and free all memory
void context_window_destroy(ContextWindow* window) {
    if (window == NULL) return;
    
    Message* current = window->head;
    while (current != NULL) {
        Message* next = current->next;
        free(current->content);
        free(current);
        current = next;
    }
    
    free(window);
}

// Create a new message object
static Message* create_message(MessageType type, MessagePriority priority, const char* content) {
    Message* msg = (Message*)malloc(sizeof(Message));
    if (msg == NULL) {
        printf("Failed to allocate memory for message\n");
        return NULL;
    }
    
    msg->type = type;
    msg->priority = priority;
    msg->content = strdup(content);
    msg->token_count = calculate_token_count(content);
    msg->next = NULL;
    msg->prev = NULL;
    
    return msg;
}

// Remove a message from the context window
static void remove_message(ContextWindow* window, Message* msg) {
    if (window == NULL || msg == NULL) return;
    
    // Update token count and message count
    window->total_tokens -= msg->token_count;
    window->message_count--;
    
    // Update linked list
    if (msg->prev != NULL) {
        msg->prev->next = msg->next;
    } else {
        window->head = msg->next;
    }
    
    if (msg->next != NULL) {
        msg->next->prev = msg->prev;
    } else {
        window->tail = msg->prev;
    }
    
    // Free memory
    free(msg->content);
    free(msg);
}

// Compress old messages to reduce token count
static bool compress_old_messages(ContextWindow* window) {
    // First try to remove low priority messages
    Message* current = window->head;
    while (current != NULL && window->total_tokens > window->max_tokens) {
        if (current->priority == PRIORITY_LOW) {
            Message* to_remove = current;
            current = current->next;
            remove_message(window, to_remove);
            continue;
        }
        current = current->next;
    }
    
    // If still over limit, remove normal priority messages
    if (window->total_tokens > window->max_tokens) {
        current = window->head;
        while (current != NULL && window->total_tokens > window->max_tokens) {
            if (current->priority == PRIORITY_NORMAL) {
                Message* to_remove = current;
                current = current->next;
                remove_message(window, to_remove);
                continue;
            }
            current = current->next;
        }
    }
    
    // If still over limit, we have to start removing high priority messages
    if (window->total_tokens > window->max_tokens) {
        current = window->head;
        while (current != NULL && window->total_tokens > window->max_tokens) {
            if (current->priority == PRIORITY_HIGH) {
                Message* to_remove = current;
                current = current->next;
                remove_message(window, to_remove);
                continue;
            }
            current = current->next;
        }
    }
    
    // If even critical messages are too much, we're in trouble
    return window->total_tokens <= window->max_tokens;
}

// Add a new message to the context window
bool context_window_add_message(ContextWindow* window,
                                MessageType type,
                                MessagePriority priority,
                                const char* content) {
    if (window == NULL || content == NULL) return false;
    
    Message* msg = create_message(type, priority, content);
    if (msg == NULL) return false;
    
    // Calculate required tokens including new message
    int new_total = window->total_tokens + msg->token_count;
    
    // If adding this message would exceed the limit, try to compress
    if (new_total > window->max_tokens) {
        if (!compress_old_messages(window)) {
            // Even after compression, still too big - check if single message fits
            if (msg->token_count > window->max_tokens) {
                printf("Error: Message is too large for context window\n");
                free(msg->content);
                free(msg);
                return false;
            }
            
            // Remove all messages to make room for this one
            while (window->total_tokens + msg->token_count > window->max_tokens) {
                if (window->head == NULL) break; // Shouldn't happen
                remove_message(window, window->head);
            }
        }
    }
    
    // Add message to the end of the linked list
    if (window->tail == NULL) {
        // Empty list
        window->head = msg;
        window->tail = msg;
    } else {
        window->tail->next = msg;
        msg->prev = window->tail;
        window->tail = msg;
    }
    
    window->total_tokens += msg->token_count;
    window->message_count++;
    
    return true;
}

// Get string representation of message type
static const char* get_message_type_string(MessageType type) {
    switch (type) {
        case MESSAGE_USER: return "User";
        case MESSAGE_ASSISTANT: return "Assistant";
        case MESSAGE_SYSTEM: return "System";
        case MESSAGE_TOOL: return "Tool";
        default: return "Unknown";
    }
}

// Get the optimized context text ready for SLM API
char* context_window_get_context(ContextWindow* window) {
    if (window == NULL || window->head == NULL) {
        char* empty = (char*)malloc(1);
        if (empty != NULL) {
            empty[0] = '\0';
        }
        return empty;
    }
    
    // Calculate buffer size
    int buffer_size = 0;
    Message* current = window->head;
    while (current != NULL) {
        // Format: "Type: Content\n"
        buffer_size += strlen(get_message_type_string(current->type)) + strlen(": ") + 
                      strlen(current->content) + strlen("\n");
        current = current->next;
    }
    
    // Allocate buffer
    char* context = (char*)malloc(buffer_size + 1);
    if (context == NULL) {
        printf("Failed to allocate memory for context string\n");
        return NULL;
    }
    
    context[0] = '\0';
    
    // Build context string
    current = window->head;
    while (current != NULL) {
        strcat(context, get_message_type_string(current->type));
        strcat(context, ": ");
        strcat(context, current->content);
        strcat(context, "\n");
        current = current->next;
    }
    
    return context;
}

// Get total number of messages in context window
int context_window_get_message_count(ContextWindow* window) {
    return window ? window->message_count : 0;
}

// Get total token count in context window
int context_window_get_token_count(ContextWindow* window) {
    return window ? window->total_tokens : 0;
}

// Print context window statistics
void context_window_print_stats(ContextWindow* window) {
    if (window == NULL) {
        printf("Context window is NULL\n");
        return;
    }
    
    printf("Context Window Statistics:\n");
    printf("Total messages: %d\n", window->message_count);
    printf("Total tokens: %d/%d\n", window->total_tokens, window->max_tokens);
    printf("Tokens remaining: %d\n", window->max_tokens - window->total_tokens);
}
