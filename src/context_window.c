#include "context_window.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define TOKEN_ESTIMATION_RATIO 4

int calculate_token_count(const char* text) {
    if (text == NULL) {
        return 0;
    }
    
    size_t length = strlen(text);
    return (int)((length + TOKEN_ESTIMATION_RATIO - 1) / TOKEN_ESTIMATION_RATIO);
}

ContextWindow* context_window_create(int max_tokens) {
    if (max_tokens <= 0) {
        fprintf(stderr, "Error: max_tokens must be positive\n");
        return NULL;
    }
    
    ContextWindow* window = (ContextWindow*)malloc(sizeof(ContextWindow));
    if (window == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for context window\n");
        return NULL;
    }
    
    window->head = NULL;
    window->tail = NULL;
    window->total_tokens = 0;
    window->max_tokens = max_tokens;
    window->message_count = 0;
    
    return window;
}

void context_window_destroy(ContextWindow* window) {
    if (window == NULL) {
        return;
    }
    
    Message* current = window->head;
    while (current != NULL) {
        Message* next = current->next;
        free(current->content);
        free(current);
        current = next;
    }
    
    free(window);
}

static Message* create_message(MessageType type, MessagePriority priority, const char* content) {
    if (content == NULL) {
        fprintf(stderr, "Error: Message content cannot be NULL\n");
        return NULL;
    }
    
    Message* msg = (Message*)malloc(sizeof(Message));
    if (msg == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for message\n");
        return NULL;
    }
    
    msg->content = strdup(content);
    if (msg->content == NULL) {
        fprintf(stderr, "Error: Failed to duplicate message content\n");
        free(msg);
        return NULL;
    }
    
    msg->type = type;
    msg->priority = priority;
    msg->token_count = calculate_token_count(content);
    msg->next = NULL;
    msg->prev = NULL;
    
    return msg;
}

static void remove_message(ContextWindow* window, Message* msg) {
    if (window == NULL || msg == NULL) {
        return;
    }
    
    window->total_tokens -= msg->token_count;
    window->message_count--;
    
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
    
    free(msg->content);
    free(msg);
}

static bool compress_old_messages(ContextWindow* window) {
    if (window == NULL) {
        return false;
    }
    
    Message* current = window->head;
    while (current != NULL && window->total_tokens > window->max_tokens) {
        Message* next = current->next;
        if (current->priority == PRIORITY_LOW) {
            remove_message(window, current);
        }
        current = next;
    }
    
    if (window->total_tokens > window->max_tokens) {
        current = window->head;
        while (current != NULL && window->total_tokens > window->max_tokens) {
            Message* next = current->next;
            if (current->priority == PRIORITY_NORMAL) {
                remove_message(window, current);
            }
            current = next;
        }
    }
    
    if (window->total_tokens > window->max_tokens) {
        current = window->head;
        while (current != NULL && window->total_tokens > window->max_tokens) {
            Message* next = current->next;
            if (current->priority == PRIORITY_HIGH) {
                remove_message(window, current);
            }
            current = next;
        }
    }
    
    return window->total_tokens <= window->max_tokens;
}

bool context_window_add_message(ContextWindow* window,
                                MessageType type,
                                MessagePriority priority,
                                const char* content) {
    if (window == NULL || content == NULL) {
        fprintf(stderr, "Error: Invalid parameters for add_message\n");
        return false;
    }
    
    Message* msg = create_message(type, priority, content);
    if (msg == NULL) {
        return false;
    }
    
    if (msg->token_count > window->max_tokens) {
        fprintf(stderr, "Error: Message (%d tokens) exceeds window capacity (%d tokens)\n",
                msg->token_count, window->max_tokens);
        free(msg->content);
        free(msg);
        return false;
    }
    
    if (window->total_tokens + msg->token_count > window->max_tokens) {
        if (!compress_old_messages(window)) {
            while (window->head != NULL && 
                   window->total_tokens + msg->token_count > window->max_tokens) {
                remove_message(window, window->head);
            }
        }
    }
    
    if (window->tail == NULL) {
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

static const char* get_message_type_string(MessageType type) {
    switch (type) {
        case MESSAGE_USER:      return "User";
        case MESSAGE_ASSISTANT: return "Assistant";
        case MESSAGE_SYSTEM:    return "System";
        case MESSAGE_TOOL:      return "Tool";
        default:                return "Unknown";
    }
}

char* context_window_get_context(ContextWindow* window) {
    if (window == NULL || window->head == NULL) {
        char* empty = (char*)malloc(1);
        if (empty != NULL) {
            empty[0] = '\0';
        }
        return empty;
    }
    
    size_t buffer_size = 0;
    Message* current = window->head;
    while (current != NULL) {
        buffer_size += strlen(get_message_type_string(current->type));
        buffer_size += strlen(": ");
        buffer_size += strlen(current->content);
        buffer_size += strlen("\n");
        current = current->next;
    }
    
    char* context = (char*)malloc(buffer_size + 1);
    if (context == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for context string\n");
        return NULL;
    }
    
    context[0] = '\0';
    
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

int context_window_get_message_count(ContextWindow* window) {
    return window ? window->message_count : 0;
}

int context_window_get_token_count(ContextWindow* window) {
    return window ? window->total_tokens : 0;
}

void context_window_print_stats(ContextWindow* window) {
    if (window == NULL) {
        printf("Context window is NULL\n");
        return;
    }
    
    printf("Context Window Statistics:\n");
    printf("  Total messages: %d\n", window->message_count);
    printf("  Total tokens: %d/%d (%.1f%% full)\n", 
           window->total_tokens, 
           window->max_tokens,
           (window->max_tokens > 0) ? (100.0 * window->total_tokens / window->max_tokens) : 0.0);
    printf("  Tokens remaining: %d\n", window->max_tokens - window->total_tokens);
}