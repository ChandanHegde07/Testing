#ifndef CONTEXT_WINDOW_H
#define CONTEXT_WINDOW_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define DEFAULT_TOKEN_RATIO 4

#define MAX_MAX_TOKENS (INT32_MAX / 2)

#define MIN_MAX_TOKENS 1

#define DEFAULT_WINDOW_SIZE 2048

typedef enum {
    MESSAGE_USER = 0,      /**< User input message */
    MESSAGE_ASSISTANT,     /**< AI/assistant response */
    MESSAGE_SYSTEM,        /**< System prompt/instructions */
    MESSAGE_TOOL           /**< Tool or function call output */
} MessageType;

typedef enum {
    PRIORITY_LOW = 0,      /**< Lowest priority - evicted first */
    PRIORITY_NORMAL,       /**< Normal/default priority */
    PRIORITY_HIGH,         /**< High priority - retained longer */
    PRIORITY_CRITICAL      /**< Highest priority - almost never evicted */
} MessagePriority;

typedef enum {
    CW_SUCCESS = 0,        /**< Operation succeeded */
    CW_ERROR_NULL_PTR,     /**< NULL pointer provided */
    CW_ERROR_INVALID_PARAM,/**< Invalid parameter value */
    CW_ERROR_NO_MEMORY,    /**< Memory allocation failed */
    CW_ERROR_FULL,         /**< Window is full */
    CW_ERROR_NOT_FOUND,    /**< Item not found */
    CW_ERROR_IO,           /**< File I/O error */
    CW_ERROR_LOCKED        /**< Resource is locked */
} CWResult;

typedef enum {
    COMPRESSION_NONE = 0,     /**< No compression, use eviction only */
    COMPRESSION_LOW_PRIORITY, /**< Remove low priority messages first */
    COMPRESSION_SUMMARIZE,     /**< Summarize old messages (future) */
    COMPRESSION_AGGRESSIVE     /**< Aggressive compression */
} CompressionStrategy;

typedef struct ContextMetrics {
    uint64_t messages_added;      /**< Total messages added */
    uint64_t messages_evicted;    /**< Total messages evicted */
    uint64_t tokens_added;        /**< Total tokens added */
    uint64_t tokens_evicted;      /**< Total tokens evicted */
    uint64_t compressions;        /**< Number of compressions performed */
    uint64_t context_retrievals;  /**< Number of context retrievals */
    double peak_utilization;      /**< Peak token utilization percentage */
    clock_t total_time;           /**< Total time window has been active */
} ContextMetrics;

typedef struct ContextConfig {
    int max_tokens;                   /**< Maximum tokens allowed */
    int min_tokens_reserve;           /**< Minimum tokens to reserve */
    CompressionStrategy compression;  /**< Compression strategy to use */
    bool enable_metrics;               /**< Enable metrics collection */
    bool thread_safe;                  /**< Enable thread safety */
    int token_ratio;                   /**< Characters per token estimate */
    bool auto_compress;                /**< Enable automatic compression */
} ContextConfig;

typedef void* ContextMutex;

typedef struct Message {
    MessageType type;              /**< Type of message */
    MessagePriority priority;      /**< Priority level */
    char* content;                /**< Message content */
    int token_count;              /**< Token count for this message */
    struct Message* next;         /**< Next message in list */
    struct Message* prev;         /**< Previous message in list */
} Message;

typedef struct ContextWindow {
    /* Linked list pointers */
    Message* head;                 /**< Head of message list */
    Message* tail;                 /**< Tail of message list */
    
    /* Token management */
    int total_tokens;              /**< Current total tokens */
    int max_tokens;               /**< Maximum allowed tokens */
    
    /* Message counts */
    int message_count;            /**< Number of messages in window */
    
    /* Configuration */
    ContextConfig config;         /**< Window configuration */
    
    /* Metrics (optional) */
    ContextMetrics* metrics;      /**< Performance metrics */
    
    /* Thread safety */
    ContextMutex* mutex;          /**< Mutex for thread safety */
} ContextWindow;

ContextWindow* context_window_create(int max_tokens);

ContextWindow* context_window_create_with_config(const ContextConfig* config);

void context_window_destroy(ContextWindow* window);

bool context_window_add_message(ContextWindow* window,
                                MessageType type,
                                MessagePriority priority,
                                const char* content);

bool context_window_add_message_ex(ContextWindow* window,
                                    MessageType type,
                                    MessagePriority priority,
                                    const char* content,
                                    CWResult* result);

bool context_window_remove_message(ContextWindow* window, const char* content);

void context_window_clear(ContextWindow* window);

char* context_window_get_context(ContextWindow* window);

char* context_window_get_context_json(ContextWindow* window);

double context_window_get_utilization(ContextWindow* window);

int context_window_get_message_count(const ContextWindow* window);

int context_window_get_token_count(const ContextWindow* window);

int context_window_get_max_tokens(const ContextWindow* window);

int context_window_get_remaining_capacity(const ContextWindow* window);

bool context_window_is_empty(const ContextWindow* window);

bool context_window_is_full(const ContextWindow* window);

int calculate_token_count(const char* text);

int calculate_token_count_ex(const char* text, int ratio);

void context_window_print_stats(ContextWindow* window);

void context_window_print_metrics(ContextWindow* window);

CWResult context_window_save(const ContextWindow* window, const char* filename);

ContextWindow* context_window_load(const char* filename);

CWResult context_window_export_json(const ContextWindow* window, const char* filename);

ContextConfig context_config_default(void);

bool context_config_validate(const ContextConfig* config);

CWResult context_window_apply_config(ContextWindow* window, const ContextConfig* config);

const ContextMetrics* context_window_get_metrics(const ContextWindow* window);

void context_window_reset_metrics(ContextWindow* window);

void context_window_set_metrics_enabled(ContextWindow* window, bool enable);

CWResult context_window_lock(ContextWindow* window);

CWResult context_window_unlock(ContextWindow* window);

bool context_window_is_thread_safe(const ContextWindow* window);

const char* context_window_version(void);

int context_window_version_major(void);

int context_window_version_minor(void);

int context_window_version_patch(void);

#endif 
