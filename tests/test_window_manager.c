#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <stdbool.h>
#include <limits.h>
#include "context_window.h"

/* Test counters */
static int tests_passed = 0;
static int tests_failed = 0;

/* Test macros */
#define TEST_START(name) do { \
    printf("  [TEST] %s... ", name); \
    fflush(stdout); \
} while(0)

#define TEST_PASS() do { \
    printf("PASS\n"); \
    tests_passed++; \
} while(0)

#define TEST_FAIL(msg) do { \
    printf("FAIL: %s\n", msg); \
    tests_failed++; \
} while(0)

#define ASSERT(cond, msg) do { \
    if (!(cond)) { \
        TEST_FAIL(msg); \
        return; \
    } \
} while(0)

void test_create_destroy_basic(void) {
    TEST_START("Create and destroy basic");
    
    ContextWindow* window = context_window_create(1000);
    ASSERT(window != NULL, "Window should not be NULL");
    ASSERT(context_window_get_message_count(window) == 0, "Message count should be 0");
    ASSERT(context_window_get_token_count(window) == 0, "Token count should be 0");
    
    context_window_destroy(window);
    
    /* Test NULL destroy (should not crash) */
    context_window_destroy(NULL);
    
    TEST_PASS();
}

void test_invalid_parameters(void) {
    TEST_START("Invalid parameter handling");
    
    /* Test create with invalid max_tokens */
    ContextWindow* window_invalid = context_window_create(0);
    ASSERT(window_invalid == NULL, "Should return NULL for 0 max_tokens");
    
    window_invalid = context_window_create(-100);
    ASSERT(window_invalid == NULL, "Should return NULL for negative max_tokens");
    
    /* Test add_message with NULL window */
    bool result = context_window_add_message(NULL, MESSAGE_USER, PRIORITY_NORMAL, "test");
    ASSERT(result == false, "Should return false for NULL window");
    
    /* Test add_message with NULL content */
    ContextWindow* window = context_window_create(1000);
    result = context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, NULL);
    ASSERT(result == false, "Should return false for NULL content");
    
    /* Test get_context with NULL window */
    char* context = context_window_get_context(NULL);
    ASSERT(context != NULL, "Should return empty string for NULL window");
    ASSERT(strlen(context) == 0, "Should return empty string");
    free(context);
    
    /* Test get_message_count with NULL window */
    ASSERT(context_window_get_message_count(NULL) == 0, "Should return 0 for NULL window");
    
    /* Test get_token_count with NULL window */
    ASSERT(context_window_get_token_count(NULL) == 0, "Should return 0 for NULL window");
    
    /* Test print_stats with NULL window (should not crash) */
    context_window_print_stats(NULL);
    
    context_window_destroy(window);
    
    TEST_PASS();
}

void test_empty_window_operations(void) {
    TEST_START("Empty window operations");
    
    ContextWindow* window = context_window_create(1000);
    ASSERT(window != NULL, "Window should not be NULL");
    
    /* Get context from empty window */
    char* context = context_window_get_context(window);
    ASSERT(context != NULL, "Context should not be NULL");
    ASSERT(strlen(context) == 0, "Context should be empty");
    free(context);
    
    /* Print stats on empty window (should not crash) */
    context_window_print_stats(window);
    
    context_window_destroy(window);
    
    TEST_PASS();
}

void test_add_single_message(void) {
    TEST_START("Add single message");
    
    ContextWindow* window = context_window_create(1000);
    
    bool result = context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, "Hello, World!");
    ASSERT(result == true, "Message should be added successfully");
    ASSERT(context_window_get_message_count(window) == 1, "Should have 1 message");
    ASSERT(context_window_get_token_count(window) > 0, "Should have positive token count");
    
    context_window_destroy(window);
    
    TEST_PASS();
}

void test_add_multiple_messages(void) {
    TEST_START("Add multiple messages");
    
    ContextWindow* window = context_window_create(1000);
    
    /* Add various message types */
    ASSERT(context_window_add_message(window, MESSAGE_SYSTEM, PRIORITY_CRITICAL, "System prompt here"), 
           "Should add system message");
    ASSERT(context_window_add_message(window, MESSAGE_USER, PRIORITY_HIGH, "User question"), 
           "Should add user message");
    ASSERT(context_window_add_message(window, MESSAGE_ASSISTANT, PRIORITY_NORMAL, "Assistant response"), 
           "Should add assistant message");
    ASSERT(context_window_add_message(window, MESSAGE_TOOL, PRIORITY_LOW, "Tool output"), 
           "Should add tool message");
    
    ASSERT(context_window_get_message_count(window) == 4, "Should have 4 messages");
    
    /* Get context and verify content */
    char* context = context_window_get_context(window);
    ASSERT(context != NULL && strlen(context) > 0, "Context should not be empty");
    free(context);
    
    context_window_destroy(window);
    
    TEST_PASS();
}

void test_token_calculation_edge_cases(void) {
    TEST_START("Token calculation edge cases");
    
    /* NULL input */
    int tokens = calculate_token_count(NULL);
    ASSERT(tokens == 0, "NULL input should return 0");
    
    /* Empty string */
    tokens = calculate_token_count("");
    ASSERT(tokens == 0, "Empty string should return 0");
    
    /* Single character */
    tokens = calculate_token_count("a");
    ASSERT(tokens == 1, "Single char should return 1");
    
    /* Exactly 4 characters (boundary) */
    tokens = calculate_token_count("abcd");
    ASSERT(tokens == 1, "4 chars should return 1 token");
    
    /* 5 characters */
    tokens = calculate_token_count("abcde");
    ASSERT(tokens == 2, "5 chars should return 2 tokens");
    
    /* Long string */
    const char* long_str = "This is a test string with multiple words to check token estimation.";
    tokens = calculate_token_count(long_str);
    ASSERT(tokens > 10, "Long string should have significant token count");
    
    /* String with special characters */
    tokens = calculate_token_count("Hello\n\t\rWorld!@#$%");
    ASSERT(tokens > 0, "Special characters should be counted");
    
    /* Unicode characters (may vary by implementation) */
    tokens = calculate_token_count("Hello 世界 🌍");
    ASSERT(tokens > 0, "Unicode should be counted");
    
    TEST_PASS();
}

void test_message_exceeds_capacity(void) {
    TEST_START("Message exceeds capacity");
    
    /* Very small window */
    ContextWindow* window = context_window_create(10);
    
    /* Try to add message larger than window */
    bool result = context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, 
        "This is a very long message that exceeds the token capacity of the window");
    ASSERT(result == false, "Should reject message exceeding capacity");
    ASSERT(context_window_get_message_count(window) == 0, "Should have no messages");
    
    context_window_destroy(window);
    
    /* Test exact boundary */
    window = context_window_create(1);
    result = context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, "ab");
    /* Should either fit or be rejected based on implementation */
    context_window_destroy(window);
    
    TEST_PASS();
}

void test_zero_max_tokens(void) {
    TEST_START("Zero max tokens");
    
    ContextWindow* window = context_window_create(0);
    ASSERT(window == NULL, "Should not create window with 0 max tokens");
    
    TEST_PASS();
}

void test_very_large_message(void) {
    TEST_START("Very large message");
    
    ContextWindow* window = context_window_create(10000);
    
    /* Create large message (1000+ chars) */
    char large_msg[2000];
    memset(large_msg, 'a', sizeof(large_msg) - 1);
    large_msg[sizeof(large_msg) - 1] = '\0';
    
    bool result = context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, large_msg);
    ASSERT(result == true, "Large message should be added");
    ASSERT(context_window_get_message_count(window) == 1, "Should have 1 message");
    
    context_window_destroy(window);
    
    TEST_PASS();
}

void test_priority_eviction_order(void) {
    TEST_START("Priority eviction order");
    
    ContextWindow* window = context_window_create(100);
    
    /* Add many low priority messages first */
    for (int i = 0; i < 10; i++) {
        char msg[50];
        snprintf(msg, sizeof(msg), "Low priority message %d", i);
        context_window_add_message(window, MESSAGE_USER, PRIORITY_LOW, msg);
    }
    
    /* Add critical system message last */
    context_window_add_message(window, MESSAGE_SYSTEM, PRIORITY_CRITICAL, "CRITICAL: Never remove this!");
    
    /* Verify system message is still present */
    char* context = context_window_get_context(window);
    bool found = (strstr(context, "CRITICAL") != NULL);
    ASSERT(found, "Critical message should be preserved");
    free(context);
    
    context_window_destroy(window);
    
    TEST_PASS();
}

void test_all_priorities_preserved(void) {
    TEST_START("All priority levels");
    
    ContextWindow* window = context_window_create(1000);
    
    ASSERT(context_window_add_message(window, MESSAGE_USER, PRIORITY_LOW, "Low priority"),
           "Should add LOW priority");
    ASSERT(context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, "Normal priority"),
           "Should add NORMAL priority");
    ASSERT(context_window_add_message(window, MESSAGE_USER, PRIORITY_HIGH, "High priority"),
           "Should add HIGH priority");
    ASSERT(context_window_add_message(window, MESSAGE_SYSTEM, PRIORITY_CRITICAL, "Critical priority"),
           "Should add CRITICAL priority");
    
    ASSERT(context_window_get_message_count(window) == 4, "Should have 4 messages");
    
    /* Verify all messages are in context */
    char* context = context_window_get_context(window);
    ASSERT(strstr(context, "Low priority") != NULL, "Should contain LOW");
    ASSERT(strstr(context, "Normal priority") != NULL, "Should contain NORMAL");
    ASSERT(strstr(context, "High priority") != NULL, "Should contain HIGH");
    ASSERT(strstr(context, "Critical priority") != NULL, "Should contain CRITICAL");
    free(context);
    
    context_window_destroy(window);
    
    TEST_PASS();
}

void test_forced_eviction(void) {
    TEST_START("Forced eviction");
    
    ContextWindow* window = context_window_create(50);
    
    /* Add messages until we need to evict */
    int added = 0;
    while (added < 20) {
        char msg[100];
        snprintf(msg, sizeof(msg), "Message number %d with some content to fill tokens", added);
        if (context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, msg)) {
            added++;
        } else {
            /* If rejected, still count it as processed */
            added++;
        }
    }
    
    /* Token count should never exceed max */
    ASSERT(context_window_get_token_count(window) <= 50, "Token count should not exceed max");
    
    context_window_destroy(window);
    
    TEST_PASS();
}

void test_sliding_window_behavior(void) {
    TEST_START("Sliding window behavior");
    
    ContextWindow* window = context_window_create(100);
    
    /* Add first message */
    context_window_add_message(window, MESSAGE_SYSTEM, PRIORITY_CRITICAL, "System 1");
    context_window_add_message(window, MESSAGE_USER, PRIORITY_HIGH, "User 1");
    context_window_add_message(window, MESSAGE_ASSISTANT, PRIORITY_NORMAL, "Assistant 1");
    
    int msg_count_1 = context_window_get_message_count(window);
    ASSERT(msg_count_1 == 3, "Should have 3 messages after initial add");
    
    /* Add more messages - should trigger sliding */
    context_window_add_message(window, MESSAGE_USER, PRIORITY_HIGH, "User 2");
    context_window_add_message(window, MESSAGE_ASSISTANT, PRIORITY_NORMAL, "Assistant 2");
    context_window_add_message(window, MESSAGE_USER, PRIORITY_HIGH, "User 3");
    context_window_add_message(window, MESSAGE_ASSISTANT, PRIORITY_NORMAL, "Assistant 3");
    
    /* Token count should still be within limit */
    ASSERT(context_window_get_token_count(window) <= 100, "Should maintain token limit");
    
    context_window_destroy(window);
    
    TEST_PASS();
}

void test_all_message_types(void) {
    TEST_START("All message types");
    
    ContextWindow* window = context_window_create(1000);
    
    ASSERT(context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, "User message"),
           "Should add USER message");
    ASSERT(context_window_add_message(window, MESSAGE_ASSISTANT, PRIORITY_NORMAL, "Assistant message"),
           "Should add ASSISTANT message");
    ASSERT(context_window_add_message(window, MESSAGE_SYSTEM, PRIORITY_CRITICAL, "System message"),
           "Should add SYSTEM message");
    ASSERT(context_window_add_message(window, MESSAGE_TOOL, PRIORITY_LOW, "Tool message"),
           "Should add TOOL message");
    
    ASSERT(context_window_get_message_count(window) == 4, "Should have 4 messages");
    
    /* Verify context contains all types */
    char* context = context_window_get_context(window);
    ASSERT(strstr(context, "User: User message") != NULL, "Should contain User type");
    ASSERT(strstr(context, "Assistant: Assistant message") != NULL, "Should contain Assistant type");
    ASSERT(strstr(context, "System: System message") != NULL, "Should contain System type");
    ASSERT(strstr(context, "Tool: Tool message") != NULL, "Should contain Tool type");
    free(context);
    
    context_window_destroy(window);
    
    TEST_PASS();
}

void test_message_type_string_format(void) {
    TEST_START("Message type string format");
    
    ContextWindow* window = context_window_create(1000);
    
    context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, "test");
    context_window_add_message(window, MESSAGE_ASSISTANT, PRIORITY_NORMAL, "test");
    context_window_add_message(window, MESSAGE_SYSTEM, PRIORITY_CRITICAL, "test");
    context_window_add_message(window, MESSAGE_TOOL, PRIORITY_LOW, "test");
    
    char* context = context_window_get_context(window);
    
    /* Check format: "Type: content" */
    ASSERT(strstr(context, "User: test") != NULL, "Should format as 'User: test'");
    ASSERT(strstr(context, "Assistant: test") != NULL, "Should format as 'Assistant: test'");
    ASSERT(strstr(context, "System: test") != NULL, "Should format as 'System: test'");
    ASSERT(strstr(context, "Tool: test") != NULL, "Should format as 'Tool: test'");
    
    free(context);
    context_window_destroy(window);
    
    TEST_PASS();
}

void test_repeated_create_destroy(void) {
    TEST_START("Repeated create/destroy");
    
    for (int i = 0; i < 100; i++) {
        ContextWindow* window = context_window_create(100);
        context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, "test");
        context_window_destroy(window);
    }
    
    /* If we get here without memory issues, test passes */
    TEST_PASS();
}

void test_many_small_messages(void) {
    TEST_START("Many small messages");
    
    ContextWindow* window = context_window_create(500);
    
    int count = 0;
    char msg[50];
    
    while (context_window_get_token_count(window) < 450) {
        snprintf(msg, sizeof(msg), "Msg%d", count);
        context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, msg);
        count++;
    }
    
    ASSERT(count > 10, "Should add many small messages");
    ASSERT(context_window_get_token_count(window) <= 500, "Should respect token limit");
    
    context_window_destroy(window);
    
    TEST_PASS();
}

void test_alternating_add_remove(void) {
    TEST_START("Alternating add/remove");
    
    ContextWindow* window = context_window_create(200);
    
    /* Add some messages */
    context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, "Message 1");
    context_window_add_message(window, MESSAGE_ASSISTANT, PRIORITY_NORMAL, "Response 1");
    
    /* Get context (simulates read) */
    char* context = context_window_get_context(window);
    ASSERT(context != NULL && strlen(context) > 0, "Should get context");
    free(context);
    
    /* Add more messages */
    context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, "Message 2");
    context_window_add_message(window, MESSAGE_ASSISTANT, PRIORITY_NORMAL, "Response 2");
    
    /* Get context again */
    context = context_window_get_context(window);
    ASSERT(context != NULL, "Should get context again");
    free(context);
    
    /* Verify structure still valid */
    ASSERT(context_window_get_token_count(window) <= 200, "Should maintain limit");
    
    context_window_destroy(window);
    
    TEST_PASS();
}

void test_extreme_token_values(void) {
    TEST_START("Extreme token values");
    
    /* Very large max_tokens */
    ContextWindow* window = context_window_create(1000000);
    ASSERT(window != NULL, "Should create window with large token limit");
    context_window_destroy(window);
    
    /* Very small but valid max_tokens */
    window = context_window_create(1);
    if (window != NULL) {
        context_window_destroy(window);
    }
    
    /* Integer overflow boundary */
    window = context_window_create(INT_MAX);
    if (window != NULL) {
        context_window_destroy(window);
    }
    
    TEST_PASS();
}

void test_print_stats(void) {
    TEST_START("Print statistics");
    
    ContextWindow* window = context_window_create(1000);
    
    /* Print on empty window */
    printf("\n    Empty window stats:\n");
    context_window_print_stats(window);
    
    /* Add some messages */
    context_window_add_message(window, MESSAGE_SYSTEM, PRIORITY_CRITICAL, "System prompt");
    context_window_add_message(window, MESSAGE_USER, PRIORITY_HIGH, "User query");
    context_window_add_message(window, MESSAGE_ASSISTANT, PRIORITY_NORMAL, "Assistant response");
    
    /* Print on populated window */
    printf("\n    Populated window stats:\n");
    context_window_print_stats(window);
    
    context_window_destroy(window);
    
    TEST_PASS();
}

void test_context_string_integrity(void) {
    TEST_START("Context string integrity");
    
    ContextWindow* window = context_window_create(1000);
    
    /* Add messages with unique content */
    const char* msg1 = "UNIQUE_CONTENT_ABC123";
    const char* msg2 = "UNIQUE_CONTENT_XYZ789";
    const char* msg3 = "UNIQUE_CONTENT_DEF456";
    
    context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, msg1);
    context_window_add_message(window, MESSAGE_ASSISTANT, PRIORITY_NORMAL, msg2);
    context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, msg3);
    
    char* context = context_window_get_context(window);
    
    /* Verify all content is present */
    ASSERT(strstr(context, msg1) != NULL, "Should contain first message");
    ASSERT(strstr(context, msg2) != NULL, "Should contain second message");
    ASSERT(strstr(context, msg3) != NULL, "Should contain third message");
    
    /* Verify proper formatting */
    int newline_count = 0;
    for (char* p = context; *p; p++) {
        if (*p == '\n') newline_count++;
    }
    ASSERT(newline_count >= 2, "Should have proper line breaks");
    
    free(context);
    context_window_destroy(window);
    
    TEST_PASS();
}

void test_stress_large_count(void) {
    TEST_START("Stress test large count");
    
    ContextWindow* window = context_window_create(10000);
    
    clock_t start = clock();
    
    /* Add many messages */
    for (int i = 0; i < 1000; i++) {
        char msg[100];
        snprintf(msg, sizeof(msg), "Stress message number %d with some additional text", i);
        context_window_add_message(window, MESSAGE_USER, i % 4, msg);
    }
    
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    
    printf("\n    Added 1000 messages in %.3f seconds", elapsed);
    
    /* Should still be within limits */
    ASSERT(context_window_get_token_count(window) <= 10000, "Should respect limit");
    
    context_window_destroy(window);
    
    TEST_PASS();
}

void test_boundary_conditions(void) {
    TEST_START("Boundary conditions");
    
    /* Test with exact fit */
    ContextWindow* window = context_window_create(1);
    if (window != NULL) {
        /* Try to add smallest possible message */
        context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, "a");
        context_window_destroy(window);
    }
    
    /* Test with very small window */
    window = context_window_create(2);
    if (window != NULL) {
        context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, "ab");
        context_window_destroy(window);
    }
    
    /* Test adding after window is full */
    window = context_window_create(5);
    if (window != NULL) {
        /* Fill the window */
        context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, "test1");
        context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, "test2");
        /* Try to add more */
        context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, "test3");
        context_window_destroy(window);
    }
    
    TEST_PASS();
}

int main(void) {
    printf("\n");
    printf("============================================================\n");
    printf("  PCC - Prompt Context Controller Test Suite\n");
    printf("============================================================\n\n");
    
    /* Run all tests */
    printf("--- Basic Functionality Tests ---\n");
    test_create_destroy_basic();
    test_invalid_parameters();
    test_empty_window_operations();
    test_add_single_message();
    test_add_multiple_messages();
    
    printf("\n--- Edge Case Tests ---\n");
    test_token_calculation_edge_cases();
    test_message_exceeds_capacity();
    test_zero_max_tokens();
    test_very_large_message();
    
    printf("\n--- Priority and Eviction Tests ---\n");
    test_priority_eviction_order();
    test_all_priorities_preserved();
    test_forced_eviction();
    test_sliding_window_behavior();
    
    printf("\n--- Message Type Tests ---\n");
    test_all_message_types();
    test_message_type_string_format();
    
    printf("\n--- Memory and Stress Tests ---\n");
    test_repeated_create_destroy();
    test_many_small_messages();
    test_alternating_add_remove();
    test_extreme_token_values();
    test_print_stats();
    test_context_string_integrity();
    test_stress_large_count();
    
    printf("\n--- Boundary Condition Tests ---\n");
    test_boundary_conditions();
    
    /* Print summary */
    printf("\n============================================================\n");
    printf("  Test Summary\n");
    printf("============================================================\n");
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("  Total:  %d\n", tests_passed + tests_failed);
    printf("============================================================\n\n");
    
    if (tests_failed > 0) {
        printf("SOME TESTS FAILED!\n\n");
        return 1;
    }
    
    printf("ALL TESTS PASSED!\n\n");
    printf("To check for memory leaks, run:\n");
    printf("  valgrind --leak-check=full ./test-window-manager\n\n");
    
    return 0;
}
