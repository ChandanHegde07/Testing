#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include "context_window.h"

/* Benchmark configuration */
#define BENCHMARK_ITERATIONS 1000
#define SMALL_WINDOW_TOKENS 500
#define MEDIUM_WINDOW_TOKENS 2000
#define LARGE_WINDOW_TOKENS 10000

/* Timing macros */
#define BENCHMARK_START(name) do { \
    printf("\n[BENCHMARK] %s\n", name); \
    start_time = clock(); \
} while(0)

#define BENCHMARK_END() do { \
    end_time = clock(); \
    elapsed = (double)(end_time - start_time) / CLOCKS_PER_SEC; \
    printf("  Time: %.4f seconds\n", elapsed); \
} while(0)

#define BENCHMARK_RESULT(label, value) do { \
    printf("  %s: %s\n", label, value); \
} while(0)

static clock_t start_time;
static clock_t end_time;
static double elapsed;

static char* generate_message(int index, int length) {
    char* msg = malloc(length + 1);
    if (msg == NULL) return NULL;
    
    /* Fill with pattern */
    snprintf(msg, length + 1, "Message %d: ", index);
    int prefix_len = strlen(msg);
    
    /* Fill remaining with 'x' */
    memset(msg + prefix_len, 'x', length - prefix_len);
    msg[length] = '\0';
    
    return msg;
}

static void benchmark_insertion(int max_tokens, int num_messages) {
    printf("\n  Window size: %d tokens, Messages: %d\n", max_tokens, num_messages);
    
    ContextWindow* window = context_window_create(max_tokens);
    if (window == NULL) {
        printf("  FAILED: Could not create window\n");
        return;
    }
    
    start_time = clock();
    
    for (int i = 0; i < num_messages; i++) {
        char* msg = generate_message(i, 50 + (i % 100));
        if (msg != NULL) {
            context_window_add_message(window, MESSAGE_USER, i % 4, msg);
            free(msg);
        }
    }
    
    end_time = clock();
    elapsed = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    printf("  Total time: %.4f seconds\n", elapsed);
    printf("  Avg per message: %.6f ms\n", (elapsed / num_messages) * 1000);
    printf("  Messages/second: %.2f\n", num_messages / elapsed);
    printf("  Final token count: %d/%d (%.1f%%)\n", 
           context_window_get_token_count(window), max_tokens,
           100.0 * context_window_get_token_count(window) / max_tokens);
    printf("  Final message count: %d\n", context_window_get_message_count(window));
    
    context_window_destroy(window);
}

static void benchmark_retrieval(int max_tokens, int num_messages) {
    printf("\n  Window size: %d tokens, Messages: %d\n", max_tokens, num_messages);
    
    ContextWindow* window = context_window_create(max_tokens);
    if (window == NULL) {
        printf("  FAILED: Could not create window\n");
        return;
    }
    
    /* Populate window */
    for (int i = 0; i < num_messages; i++) {
        char* msg = generate_message(i, 50 + (i % 100));
        if (msg != NULL) {
            context_window_add_message(window, MESSAGE_USER, i % 4, msg);
            free(msg);
        }
    }
    
    /* Benchmark retrieval */
    start_time = clock();
    
    char* context = NULL;
    for (int i = 0; i < 100; i++) {
        free(context);
        context = context_window_get_context(window);
    }
    
    end_time = clock();
    elapsed = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    printf("  100 retrievals time: %.4f seconds\n", elapsed);
    printf("  Avg per retrieval: %.4f ms\n", (elapsed / 100) * 1000);
    printf("  Retrievals/second: %.2f\n", 100 / elapsed);
    printf("  Context length: %zu bytes\n", context ? strlen(context) : 0);
    
    free(context);
    context_window_destroy(window);
}

static void benchmark_stress(int max_tokens) {
    printf("\n  Window size: %d tokens, Rapid additions\n", max_tokens);
    
    ContextWindow* window = context_window_create(max_tokens);
    if (window == NULL) {
        printf("  FAILED: Could not create window\n");
        return;
    }
    
    start_time = clock();
    
    int added = 0;
    for (int i = 0; i < 10000; i++) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Stress test message %d", i);
        if (context_window_add_message(window, MESSAGE_USER, i % 4, msg)) {
            added++;
        }
    }
    
    end_time = clock();
    elapsed = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    printf("  Messages attempted: 10000\n");
    printf("  Messages added: %d\n", added);
    printf("  Total time: %.4f seconds\n", elapsed);
    printf("  Avg per message: %.6f ms\n", (elapsed / 10000) * 1000);
    printf("  Final tokens: %d/%d\n", 
           context_window_get_token_count(window), max_tokens);
    
    context_window_destroy(window);
}

static void benchmark_utilization(int max_tokens) {
    printf("\n  Window size: %d tokens\n", max_tokens);
    
    ContextWindow* window = context_window_create(max_tokens);
    if (window == NULL) {
        printf("  FAILED: Could not create window\n");
        return;
    }
    
    /* Add many small messages */
    int small_count = 0;
    while (context_window_get_token_count(window) < max_tokens * 0.9) {
        char msg[32];
        snprintf(msg, sizeof(msg), "Msg%d", small_count++);
        context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, msg);
    }
    
    int count = context_window_get_message_count(window);
    int tokens = context_window_get_token_count(window);
    double utilization = 100.0 * tokens / max_tokens;
    
    printf("  Small messages added: %d\n", small_count);
    printf("  Final message count: %d\n", count);
    printf("  Token utilization: %.1f%%\n", utilization);
    printf("  Tokens per message: %.2f\n", (double)tokens / count);
    
    context_window_destroy(window);
}

static void benchmark_eviction(int max_tokens) {
    printf("\n  Window size: %d tokens\n", max_tokens);
    
    ContextWindow* window = context_window_create(max_tokens);
    if (window == NULL) {
        printf("  FAILED: Could not create window\n");
        return;
    }
    
    /* Fill window */
    for (int i = 0; i < 100; i++) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Message %d with some content", i);
        context_window_add_message(window, MESSAGE_USER, i % 4, msg);
    }
    
    int initial_count = context_window_get_message_count(window);
    int initial_tokens = context_window_get_token_count(window);
    
    /* Force more additions to trigger eviction */
    start_time = clock();
    
    for (int i = 0; i < 100; i++) {
        char msg[64];
        snprintf(msg, sizeof(msg), "New message %d that should trigger eviction", i);
        context_window_add_message(window, MESSAGE_USER, i % 4, msg);
    }
    
    end_time = clock();
    elapsed = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    int final_count = context_window_get_message_count(window);
    int final_tokens = context_window_get_token_count(window);
    
    printf("  Initial: %d messages, %d tokens\n", initial_count, initial_tokens);
    printf("  Final: %d messages, %d tokens\n", final_count, final_tokens);
    printf("  Evictions: %d\n", initial_count - final_count + (100 - (final_count < initial_count ? 0 : 0)));
    printf("  Time for 100 additions: %.4f seconds\n", elapsed);
    
    context_window_destroy(window);
}

static void benchmark_mixed_operations(int max_tokens) {
    printf("\n  Window size: %d tokens\n", max_tokens);
    
    ContextWindow* window = context_window_create(max_tokens);
    if (window == NULL) {
        printf("  FAILED: Could not create window\n");
        return;
    }
    
    start_time = clock();
    
    /* Mixed operations */
    for (int i = 0; i < 500; i++) {
        /* Add message */
        char msg[64];
        snprintf(msg, sizeof(msg), "Message %d", i);
        context_window_add_message(window, MESSAGE_USER, i % 4, msg);
        
        /* Every 10th iteration, retrieve context */
        if (i % 10 == 0) {
            char* context = context_window_get_context(window);
            free(context);
        }
    }
    
    end_time = clock();
    elapsed = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    printf("  500 mixed operations time: %.4f seconds\n", elapsed);
    printf("  Ops/second: %.2f\n", 500 / elapsed);
    printf("  Final: %d messages, %d tokens\n", 
           context_window_get_message_count(window),
           context_window_get_token_count(window));
    
    context_window_destroy(window);
}

static void benchmark_message_types(int max_tokens) {
    printf("\n  Window size: %d tokens\n", max_tokens);
    
    ContextWindow* window = context_window_create(max_tokens);
    if (window == NULL) {
        printf("  FAILED: Could not create window\n");
        return;
    }
    
    start_time = clock();
    
    /* Add all message types */
    for (int i = 0; i < 250; i++) {
        context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, "User message");
        context_window_add_message(window, MESSAGE_ASSISTANT, PRIORITY_NORMAL, "Assistant response");
        context_window_add_message(window, MESSAGE_SYSTEM, PRIORITY_CRITICAL, "System prompt");
        context_window_add_message(window, MESSAGE_TOOL, PRIORITY_LOW, "Tool output");
    }
    
    end_time = clock();
    elapsed = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    printf("  1000 messages (all types): %.4f seconds\n", elapsed);
    printf("  Final: %d messages, %d tokens\n", 
           context_window_get_message_count(window),
           context_window_get_token_count(window));
    
    context_window_destroy(window);
}

int main(void) {
    printf("============================================================\n");
    printf("  PCC - Prompt Context Controller Performance Benchmarks\n");
    printf("============================================================\n");
    printf("\nCompiler: %s\n", __VERSION__);
    printf("Date: %s %s\n", __DATE__, __TIME__);
    
    /* Warm-up run */
    printf("\n--- Warm-up ---\n");
    ContextWindow* warmup = context_window_create(100);
    for (int i = 0; i < 10; i++) {
        char msg[32];
        snprintf(msg, sizeof(msg), "Warmup %d", i);
        context_window_add_message(warmup, MESSAGE_USER, PRIORITY_NORMAL, msg);
    }
    context_window_destroy(warmup);
    printf("  Warm-up complete\n");
    
    /* Insertion benchmarks */
    printf("\n============================================================\n");
    printf("  INSERTION BENCHMARKS\n");
    printf("============================================================\n");
    
    benchmark_insertion(SMALL_WINDOW_TOKENS, 100);
    benchmark_insertion(SMALL_WINDOW_TOKENS, 500);
    benchmark_insertion(MEDIUM_WINDOW_TOKENS, 1000);
    benchmark_insertion(LARGE_WINDOW_TOKENS, 5000);
    
    /* Retrieval benchmarks */
    printf("\n============================================================\n");
    printf("  RETRIEVAL BENCHMARKS\n");
    printf("============================================================\n");
    
    benchmark_retrieval(SMALL_WINDOW_TOKENS, 50);
    benchmark_retrieval(MEDIUM_WINDOW_TOKENS, 200);
    benchmark_retrieval(LARGE_WINDOW_TOKENS, 1000);
    
    /* Stress test */
    printf("\n============================================================\n");
    printf("  STRESS TEST\n");
    printf("============================================================\n");
    
    benchmark_stress(SMALL_WINDOW_TOKENS);
    benchmark_stress(MEDIUM_WINDOW_TOKENS);
    
    /* Utilization test */
    printf("\n============================================================\n");
    printf("  TOKEN UTILIZATION\n");
    printf("============================================================\n");
    
    benchmark_utilization(SMALL_WINDOW_TOKENS);
    benchmark_utilization(MEDIUM_WINDOW_TOKENS);
    benchmark_utilization(LARGE_WINDOW_TOKENS);
    
    /* Eviction test */
    printf("\n============================================================\n");
    printf("  EVICTION PERFORMANCE\n");
    printf("============================================================\n");
    
    benchmark_eviction(SMALL_WINDOW_TOKENS);
    benchmark_eviction(MEDIUM_WINDOW_TOKENS);
    
    /* Mixed operations */
    printf("\n============================================================\n");
    printf("  MIXED OPERATIONS\n");
    printf("============================================================\n");
    
    benchmark_mixed_operations(SMALL_WINDOW_TOKENS);
    benchmark_mixed_operations(MEDIUM_WINDOW_TOKENS);
    
    /* Message types */
    printf("\n============================================================\n");
    printf("  MESSAGE TYPE PERFORMANCE\n");
    printf("============================================================\n");
    
    benchmark_message_types(MEDIUM_WINDOW_TOKENS);
    
    /* Summary */
    printf("\n============================================================\n");
    printf("  BENCHMARK COMPLETE\n");
    printf("============================================================\n\n");
    
    return 0;
}
