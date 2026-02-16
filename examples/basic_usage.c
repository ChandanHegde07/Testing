#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "context_window.h"

int main(void) {
    printf("========================================\n");
    printf("  PCC - Basic Usage Example\n");
    printf("========================================\n\n");
    
    /* Create a context window with 1000 token limit */
    ContextWindow* window = context_window_create(1000);
    if (window == NULL) {
        fprintf(stderr, "Failed to create context window\n");
        return 1;
    }
    
    printf("Created context window with 1000 token limit\n\n");
    
    /* Add system prompt (CRITICAL priority - never evicted) */
    printf("Adding system prompt (CRITICAL priority)...\n");
    context_window_add_message(window, MESSAGE_SYSTEM, PRIORITY_CRITICAL,
        "You are a helpful AI assistant. Provide accurate and concise answers.");
    
    /* Add user message (HIGH priority) */
    printf("Adding user message (HIGH priority)...\n");
    context_window_add_message(window, MESSAGE_USER, PRIORITY_HIGH,
        "What is the capital of France?");
    
    /* Add assistant response (NORMAL priority) */
    printf("Adding assistant response (NORMAL priority)...\n");
    context_window_add_message(window, MESSAGE_ASSISTANT, PRIORITY_NORMAL,
        "The capital of France is Paris.");
    
    /* Add another user message */
    printf("Adding another user message...\n");
    context_window_add_message(window, MESSAGE_USER, PRIORITY_HIGH,
        "What about Germany?");
    
    /* Add another assistant response */
    printf("Adding another assistant response...\n");
    context_window_add_message(window, MESSAGE_ASSISTANT, PRIORITY_NORMAL,
        "The capital of Germany is Berlin.");
    
    /* Print statistics */
    printf("\n");
    context_window_print_stats(window);
    
    /* Print metrics if enabled */
    printf("\n");
    context_window_print_metrics(window);
    
    /* Get the formatted context for SLM API */
    printf("\n--- Context for SLM API ---\n");
    char* context = context_window_get_context(window);
    if (context != NULL) {
        printf("%s", context);
        free(context);
    }
    
    /* Demonstrate utilization */
    printf("\n--- Utilization ---\n");
    printf("Current utilization: %.1f%%\n", context_window_get_utilization(window));
    printf("Messages in window: %d\n", context_window_get_message_count(window));
    printf("Tokens in window: %d\n", context_window_get_token_count(window));
    printf("Remaining capacity: %d tokens\n", context_window_get_remaining_capacity(window));
    
    /* Cleanup */
    context_window_destroy(window);
    
    printf("\n========================================\n");
    printf("  Example completed successfully!\n");
    printf("========================================\n");
    
    return 0;
}
