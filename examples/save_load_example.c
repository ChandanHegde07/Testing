#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "context_window.h"

#define SAVE_FILE "context_save.txt"
#define SAVE_JSON_FILE "context_save.json"

int main(void) {
    printf("========================================\n");
    printf("  PCC - Save/Load Example\n");
    printf("========================================\n\n");
    
    /* Create and populate a context window */
    printf("Creating and populating context window...\n");
    ContextWindow* window = context_window_create(2000);
    if (window == NULL) {
        fprintf(stderr, "Failed to create context window\n");
        return 1;
    }
    
    /* Add some messages */
    context_window_add_message(window, MESSAGE_SYSTEM, PRIORITY_CRITICAL,
        "You are a helpful AI assistant specializing in C programming.");
    context_window_add_message(window, MESSAGE_USER, PRIORITY_HIGH,
        "How do I allocate memory in C?");
    context_window_add_message(window, MESSAGE_ASSISTANT, PRIORITY_NORMAL,
        "In C, you can allocate memory dynamically using malloc(), calloc(), or realloc().");
    context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL,
        "What's the difference between them?");
    context_window_add_message(window, MESSAGE_ASSISTANT, PRIORITY_NORMAL,
        "malloc() allocates uninitialized memory, calloc() zeros the memory, "
        "and realloc() resizes existing allocation.");
    
    printf("Populated with %d messages (%d tokens)\n\n",
           context_window_get_message_count(window),
           context_window_get_token_count(window));
    
    /* Save to file */
    printf("Saving to file: %s\n", SAVE_FILE);
    CWResult result = context_window_save(window, SAVE_FILE);
    if (result == CW_SUCCESS) {
        printf("  Successfully saved!\n");
    } else {
        printf("  Failed to save (error code: %d)\n", result);
    }
    
    /* Export to JSON */
    printf("\nExporting to JSON: %s\n", SAVE_JSON_FILE);
    result = context_window_export_json(window, SAVE_JSON_FILE);
    if (result == CW_SUCCESS) {
        printf("  Successfully exported!\n");
    } else {
        printf("  Failed to export (error code: %d)\n", result);
    }
    
    /* Destroy original window */
    printf("\nDestroying original window...\n");
    context_window_destroy(window);
    
    /* Load from file */
    printf("\nLoading from file: %s\n", SAVE_FILE);
    window = context_window_load(SAVE_FILE);
    if (window == NULL) {
        fprintf(stderr, "  Failed to load!\n");
        return 1;
    }
    
    printf("  Successfully loaded!\n");
    printf("  Messages: %d\n", context_window_get_message_count(window));
    printf("  Tokens: %d\n", context_window_get_token_count(window));
    
    /* Display loaded context */
    printf("\n--- Loaded Context ---\n");
    char* context = context_window_get_context(window);
    if (context != NULL) {
        printf("%s", context);
        free(context);
    }
    
    /* Show statistics */
    printf("\n--- Statistics ---\n");
    context_window_print_stats(window);
    
    /* Cleanup */
    context_window_destroy(window);
    
    /* Show JSON file contents */
    printf("\n--- JSON File Contents ---\n");
    printf("(See %s for full JSON output)\n", SAVE_JSON_FILE);
    
    printf("\n========================================\n");
    printf("  Example completed successfully!\n");
    printf("========================================\n");
    
    return 0;
}
