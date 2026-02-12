// SLM Context Window Manager Tests
// Test file for window management functionality

#include <stdio.h>
#include <assert.h>
#include "context_window.h"

#define TEST_MAX_TOKENS 500

void test_create_destroy() {
    printf("Test 1: Create and destroy context window... ");
    
    ContextWindow* window = context_window_create(TEST_MAX_TOKENS);
    assert(window != NULL);
    assert(context_window_get_message_count(window) == 0);
    assert(context_window_get_token_count(window) == 0);
    
    context_window_destroy(window);
    
    printf("PASS\n");
}

void test_add_messages() {
    printf("Test 2: Add messages... ");
    
    ContextWindow* window = context_window_create(TEST_MAX_TOKENS);
    
    // Add simple messages
    assert(context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, "Hello"));
    assert(context_window_add_message(window, MESSAGE_ASSISTANT, PRIORITY_NORMAL, "Hi there"));
    assert(context_window_add_message(window, MESSAGE_SYSTEM, PRIORITY_CRITICAL, "System message"));
    
    assert(context_window_get_message_count(window) == 3);
    assert(context_window_get_token_count(window) > 0);
    
    context_window_destroy(window);
    
    printf("PASS\n");
}

void test_token_calculation() {
    printf("Test 3: Token calculation... ");
    
    int tokens1 = calculate_token_count("Hello, world!");
    int tokens2 = calculate_token_count("This is a longer message with more words to test token calculation.");
    
    assert(tokens1 > 0);
    assert(tokens2 > tokens1);
    
    printf("PASS\n");
}

void test_sliding_window() {
    printf("Test 4: Sliding window behavior... ");
    
    ContextWindow* window = context_window_create(TEST_MAX_TOKENS);
    
    // Add messages until we exceed the limit
    for (int i = 0; i < 20; i++) {
        char content[100];
        sprintf(content, "Message %d: This is a test message to fill up the context window.", i);
        context_window_add_message(window, MESSAGE_USER, i % 4, content);
    }
    
    // Verify we didn't exceed the token limit
    assert(context_window_get_token_count(window) <= TEST_MAX_TOKENS);
    
    context_window_destroy(window);
    
    printf("PASS\n");
}

void test_priority_handling() {
    printf("Test 5: Priority-based retention... ");
    
    ContextWindow* window = context_window_create(TEST_MAX_TOKENS);
    
    // Add messages with varying priorities
    for (int i = 0; i < 15; i++) {
        char content[100];
        sprintf(content, "Priority %d message %d", i % 4, i);
        context_window_add_message(window, MESSAGE_USER, i % 4, content);
    }
    
    // Check that critical messages are still present
    bool found_critical = false;
    Message* current = window->head;
    while (current != NULL) {
        if (current->priority == PRIORITY_CRITICAL) {
            found_critical = true;
            break;
        }
        current = current->next;
    }
    
    assert(found_critical);
    
    context_window_destroy(window);
    
    printf("PASS\n");
}

void test_get_context() {
    printf("Test 6: Get context string... ");
    
    ContextWindow* window = context_window_create(TEST_MAX_TOKENS);
    
    context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, "Hello");
    context_window_add_message(window, MESSAGE_ASSISTANT, PRIORITY_NORMAL, "Hi there");
    
    char* context = context_window_get_context(window);
    assert(context != NULL);
    assert(strlen(context) > 0);
    
    free(context);
    context_window_destroy(window);
    
    printf("PASS\n");
}

int main() {
    printf("SLM Context Window Manager - Test Suite\n");
    printf("======================================\n\n");
    
    test_create_destroy();
    test_add_messages();
    test_token_calculation();
    test_sliding_window();
    test_priority_handling();
    test_get_context();
    
    printf("\n======================================\n");
    printf("All tests PASSED!\n");
    
    return 0;
}
