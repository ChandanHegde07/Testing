#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "context_window.h"

int main(void) {
    printf("========================================\n");
    printf("  PCC - Configuration Example\n");
    printf("========================================\n\n");
    
    /* Get default configuration */
    printf("1. Using default configuration:\n");
    ContextConfig default_config = context_config_default();
    printf("   max_tokens: %d\n", default_config.max_tokens);
    printf("   token_ratio: %d\n", default_config.token_ratio);
    printf("   enable_metrics: %s\n", default_config.enable_metrics ? "true" : "false");
    printf("   thread_safe: %s\n", default_config.thread_safe ? "true" : "false");
    printf("   auto_compress: %s\n", default_config.auto_compress ? "true" : "false");
    printf("   compression: %d\n", default_config.compression);
    
    /* Create window with default config */
    ContextWindow* window = context_window_create_with_config(&default_config);
    if (window == NULL) {
        fprintf(stderr, "Failed to create context window\n");
        return 1;
    }
    
    /* Add some messages */
    for (int i = 0; i < 5; i++) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Message %d with some content", i);
        context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, msg);
    }
    
    printf("\n2. With default settings:\n");
    context_window_print_stats(window);
    
    /* Modify configuration dynamically */
    printf("\n3. Applying new configuration...\n");
    ContextConfig new_config = context_config_default();
    new_config.max_tokens = 500;  /* Reduce token limit */
    new_config.compression = COMPRESSION_AGGRESSIVE;
    new_config.auto_compress = true;
    
    CWResult result = context_window_apply_config(window, &new_config);
    if (result == CW_SUCCESS) {
        printf("   Configuration applied successfully!\n");
    } else {
        printf("   Failed to apply configuration (error: %d)\n", result);
    }
    
    printf("\n4. After reducing max_tokens to 500:\n");
    context_window_print_stats(window);
    
    /* Demonstrate metrics */
    printf("\n5. Metrics tracking:\n");
    context_window_print_metrics(window);
    
    /* Reset metrics */
    printf("\n6. Resetting metrics...\n");
    context_window_reset_metrics(window);
    
    /* Add more messages to generate new metrics */
    for (int i = 0; i < 10; i++) {
        char msg[128];
        snprintf(msg, sizeof(msg), "New message %d", i);
        context_window_add_message(window, MESSAGE_USER, PRIORITY_LOW, msg);
    }
    
    printf("\n7. After adding more messages:\n");
    context_window_print_metrics(window);
    
    /* Demonstrate validation */
    printf("\n8. Configuration validation:\n");
    ContextConfig invalid_config = context_config_default();
    invalid_config.max_tokens = -100;
    invalid_config.token_ratio = 0;
    
    if (context_config_validate(&invalid_config)) {
        printf("   Invalid config validated (unexpected!)\n");
    } else {
        printf("   Invalid config correctly rejected!\n");
    }
    
    /* Cleanup */
    context_window_destroy(window);
    
    printf("\n========================================\n");
    printf("  Example completed successfully!\n");
    printf("========================================\n");
    
    return 0;
}
