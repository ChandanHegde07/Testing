#include "context_window.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>

static Message* create_message(MessageType type, MessagePriority priority, const char* content, int token_ratio);
static void remove_message(ContextWindow* window, Message* msg);
static bool compress_old_messages(ContextWindow* window);
static const char* get_message_type_string(MessageType type);
static const char* get_message_priority_string(MessagePriority priority);
static void init_metrics(ContextWindow* window);
static void update_metrics_on_add(ContextWindow* window, int tokens);
static void update_metrics_on_evict(ContextWindow* window, int tokens);
static CWResult write_message_to_file(FILE* fp, const Message* msg);
static Message* read_message_from_file(FILE* fp);

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 0

const char* context_window_version(void) {
    return "1.0.0";
}

int context_window_version_major(void) {
    return VERSION_MAJOR;
}

int context_window_version_minor(void) {
    return VERSION_MINOR;
}

int context_window_version_patch(void) {
    return VERSION_PATCH;
}

int calculate_token_count(const char* text) {
    return calculate_token_count_ex(text, DEFAULT_TOKEN_RATIO);
}

int calculate_token_count_ex(const char* text, int ratio) {
    if (text == NULL || ratio <= 0) {
        return 0;
    }
    
    size_t length = strlen(text);
    return (int)((length + ratio - 1) / ratio);
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

static const char* get_message_priority_string(MessagePriority priority) {
    switch (priority) {
        case PRIORITY_LOW:      return "LOW";
        case PRIORITY_NORMAL:   return "NORMAL";
        case PRIORITY_HIGH:     return "HIGH";
        case PRIORITY_CRITICAL: return "CRITICAL";
        default:                return "UNKNOWN";
    }
}

static void init_metrics(ContextWindow* window) {
    if (window == NULL || !window->config.enable_metrics) {
        return;
    }
    
    window->metrics = (ContextMetrics*)calloc(1, sizeof(ContextMetrics));
    if (window->metrics != NULL) {
        window->metrics->total_time = clock();
    }
}

static void update_metrics_on_add(ContextWindow* window, int tokens) {
    if (window == NULL || window->metrics == NULL) {
        return;
    }
    
    window->metrics->messages_added++;
    window->metrics->tokens_added += tokens;
    
    double utilization = 100.0 * window->total_tokens / window->max_tokens;
    if (utilization > window->metrics->peak_utilization) {
        window->metrics->peak_utilization = utilization;
    }
}

static void update_metrics_on_evict(ContextWindow* window, int tokens) {
    if (window == NULL || window->metrics == NULL) {
        return;
    }
    
    window->metrics->messages_evicted++;
    window->metrics->tokens_evicted += tokens;
}

static Message* create_message(MessageType type, MessagePriority priority, const char* content, int token_ratio) {
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
    msg->token_count = calculate_token_count_ex(content, token_ratio);
    msg->next = NULL;
    msg->prev = NULL;
    
    return msg;
}

static void remove_message(ContextWindow* window, Message* msg) {
    if (window == NULL || msg == NULL) {
        return;
    }
    
    /* Update metrics before removing */
    update_metrics_on_evict(window, msg->token_count);
    
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
    
    if (window->config.compression == COMPRESSION_NONE) {
        return false;
    }
    
    /* Phase 1: Remove LOW priority messages */
    Message* current = window->head;
    while (current != NULL && window->total_tokens > window->max_tokens) {
        Message* next = current->next;
        if (current->priority == PRIORITY_LOW) {
            remove_message(window, current);
            if (window->metrics != NULL) {
                window->metrics->compressions++;
            }
        }
        current = next;
    }
    
    /* Phase 2: Remove NORMAL priority if still over limit */
    if (window->total_tokens > window->max_tokens) {
        current = window->head;
        while (current != NULL && window->total_tokens > window->max_tokens) {
            Message* next = current->next;
            if (current->priority == PRIORITY_NORMAL) {
                remove_message(window, current);
                if (window->metrics != NULL) {
                    window->metrics->compressions++;
                }
            }
            current = next;
        }
    }
    
    /* Phase 3: Remove HIGH priority (critical only kept) */
    if (window->total_tokens > window->max_tokens) {
        current = window->head;
        while (current != NULL && window->total_tokens > window->max_tokens) {
            Message* next = current->next;
            if (current->priority == PRIORITY_HIGH) {
                remove_message(window, current);
                if (window->metrics != NULL) {
                    window->metrics->compressions++;
                }
            }
            current = next;
        }
    }
    
    return window->total_tokens <= window->max_tokens;
}

ContextWindow* context_window_create(int max_tokens) {
    ContextConfig config = context_config_default();
    config.max_tokens = max_tokens;
    return context_window_create_with_config(&config);
}

ContextWindow* context_window_create_with_config(const ContextConfig* config) {
    /* Validate configuration */
    if (config == NULL) {
        fprintf(stderr, "Error: Configuration cannot be NULL\n");
        return NULL;
    }
    
    if (!context_config_validate(config)) {
        fprintf(stderr, "Error: Invalid configuration\n");
        return NULL;
    }
    
    ContextWindow* window = (ContextWindow*)malloc(sizeof(ContextWindow));
    if (window == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for context window\n");
        return NULL;
    }
    
    /* Initialize to zero */
    memset(window, 0, sizeof(ContextWindow));
    
    /* Copy configuration */
    window->config = *config;
    window->max_tokens = config->max_tokens;
    
    /* Full pthread implementation would go here */
    (void)config;  /* Suppress unused warning */
    
    /* Initialize metrics if enabled */
    init_metrics(window);
    
    return window;
}

void context_window_destroy(ContextWindow* window) {
    if (window == NULL) {
        return;
    }
    
    /* Free all messages */
    Message* current = window->head;
    while (current != NULL) {
        Message* next = current->next;
        free(current->content);
        free(current);
        current = next;
    }
    
    /* Free metrics */
    if (window->metrics != NULL) {
        free(window->metrics);
    }
    
    /* Thread mutex - not yet implemented */
    
    free(window);
}

bool context_window_add_message(ContextWindow* window,
                                MessageType type,
                                MessagePriority priority,
                                const char* content) {
    CWResult result;
    return context_window_add_message_ex(window, type, priority, content, &result);
}

bool context_window_add_message_ex(ContextWindow* window,
                                    MessageType type,
                                    MessagePriority priority,
                                    const char* content,
                                    CWResult* result) {
    /* Validate inputs */
    if (window == NULL || content == NULL) {
        if (result) *result = CW_ERROR_NULL_PTR;
        fprintf(stderr, "Error: Invalid parameters for add_message\n");
        return false;
    }
    
    /* Create message */
    Message* msg = create_message(type, priority, content, window->config.token_ratio);
    if (msg == NULL) {
        if (result) *result = CW_ERROR_NO_MEMORY;
        return false;
    }
    
    /* Check if message exceeds window capacity */
    if (msg->token_count > window->max_tokens) {
        fprintf(stderr, "Error: Message (%d tokens) exceeds window capacity (%d tokens)\n",
                msg->token_count, window->max_tokens);
        free(msg->content);
        free(msg);
        if (result) *result = CW_ERROR_FULL;
        return false;
    }
    
    /* Handle overflow */
    if (window->total_tokens + msg->token_count > window->max_tokens) {
        if (window->config.auto_compress) {
            compress_old_messages(window);
        }
        
        /* Force eviction if still over limit */
        while (window->head != NULL && 
               window->total_tokens + msg->token_count > window->max_tokens) {
            remove_message(window, window->head);
        }
    }
    
    /* Add message to tail */
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
    
    /* Update metrics */
    update_metrics_on_add(window, msg->token_count);
    
    if (result) *result = CW_SUCCESS;
    return true;
}

bool context_window_remove_message(ContextWindow* window, const char* content) {
    if (window == NULL || content == NULL) {
        return false;
    }
    
    Message* current = window->head;
    while (current != NULL) {
        if (strcmp(current->content, content) == 0) {
            remove_message(window, current);
            return true;
        }
        current = current->next;
    }
    
    return false;
}

void context_window_clear(ContextWindow* window) {
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
    
    window->head = NULL;
    window->tail = NULL;
    window->total_tokens = 0;
    window->message_count = 0;
}

char* context_window_get_context(ContextWindow* window) {
    if (window == NULL || window->head == NULL) {
        char* empty = (char*)malloc(1);
        if (empty != NULL) {
            empty[0] = '\0';
        }
        return empty;
    }
    
    /* Update metrics */
    if (window->metrics != NULL) {
        window->metrics->context_retrievals++;
    }
    
    /* Calculate buffer size */
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

char* context_window_get_context_json(ContextWindow* window) {
    if (window == NULL) {
        return strdup("[]");
    }
    
    /* Calculate size needed */
    size_t size = 3;  /* "[]\0" */
    Message* current = window->head;
    while (current != NULL) {
        size += 50;  /* JSON formatting overhead */
        size += strlen(current->content) * 2;  /* Escape characters */
        current = current->next;
    }
    
    char* json = (char*)malloc(size);
    if (json == NULL) {
        return NULL;
    }
    
    strcpy(json, "[\n");
    current = window->head;
    
    while (current != NULL) {
        char* escaped = (char*)malloc(strlen(current->content) * 2 + 1);
        if (escaped != NULL) {
            /* Simple escape (in production, use proper JSON escaping) */
            char* dst = escaped;
            const char* src = current->content;
            while (*src) {
                if (*src == '"' || *src == '\\') {
                    *dst++ = '\\';
                }
                *dst++ = *src++;
            }
            *dst = '\0';
            
            strcat(json, "  {\n");
            char type_str[32];
            snprintf(type_str, sizeof(type_str), "\"type\": \"%s\"", 
                     get_message_type_string(current->type));
            strcat(json, type_str);
            strcat(json, ",\n");
            
            char prio_str[32];
            snprintf(prio_str, sizeof(prio_str), "\"priority\": \"%s\"", 
                     get_message_priority_string(current->priority));
            strcat(json, prio_str);
            strcat(json, ",\n");
            
            strcat(json, "\"content\": \"");
            strcat(json, escaped);
            strcat(json, "\",\n");
            
            char tokens_str[32];
            snprintf(tokens_str, sizeof(tokens_str), "\"tokens\": %d", 
                     current->token_count);
            strcat(json, tokens_str);
            strcat(json, "\n  }");
            
            if (current->next != NULL) {
                strcat(json, ",");
            }
            strcat(json, "\n");
            
            free(escaped);
        }
        current = current->next;
    }
    
    strcat(json, "]");
    
    return json;
}

double context_window_get_utilization(ContextWindow* window) {
    if (window == NULL || window->max_tokens == 0) {
        return 0.0;
    }
    
    return 100.0 * window->total_tokens / window->max_tokens;
}

int context_window_get_message_count(const ContextWindow* window) {
    return window ? window->message_count : 0;
}

int context_window_get_token_count(const ContextWindow* window) {
    return window ? window->total_tokens : 0;
}

int context_window_get_max_tokens(const ContextWindow* window) {
    return window ? window->max_tokens : 0;
}

int context_window_get_remaining_capacity(const ContextWindow* window) {
    if (window == NULL) {
        return 0;
    }
    
    int remaining = window->max_tokens - window->total_tokens;
    return remaining > 0 ? remaining : 0;
}

bool context_window_is_empty(const ContextWindow* window) {
    return window == NULL || window->message_count == 0;
}

bool context_window_is_full(const ContextWindow* window) {
    if (window == NULL) {
        return false;
    }
    
    return window->total_tokens >= window->max_tokens;
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
           context_window_get_utilization(window));
    printf("  Tokens remaining: %d\n", window->max_tokens - window->total_tokens);
    printf("  Thread safe: %s\n", window->config.thread_safe ? "Yes" : "No");
    printf("  Metrics enabled: %s\n", window->config.enable_metrics ? "Yes" : "No");
}

void context_window_print_metrics(ContextWindow* window) {
    if (window == NULL) {
        printf("Context window is NULL\n");
        return;
    }
    
    if (window->metrics == NULL) {
        printf("Metrics not enabled for this window\n");
        return;
    }
    
    printf("Context Window Metrics:\n");
    printf("  Messages added: %lu\n", (unsigned long)window->metrics->messages_added);
    printf("  Messages evicted: %lu\n", (unsigned long)window->metrics->messages_evicted);
    printf("  Tokens added: %lu\n", (unsigned long)window->metrics->tokens_added);
    printf("  Tokens evicted: %lu\n", (unsigned long)window->metrics->tokens_evicted);
    printf("  Compressions: %lu\n", (unsigned long)window->metrics->compressions);
    printf("  Context retrievals: %lu\n", (unsigned long)window->metrics->context_retrievals);
    printf("  Peak utilization: %.1f%%\n", window->metrics->peak_utilization);
    
    if (window->metrics->total_time > 0) {
        double uptime = (double)(clock() - window->metrics->total_time) / CLOCKS_PER_SEC;
        printf("  Active time: %.2f seconds\n", uptime);
    }
}

static CWResult write_message_to_file(FILE* fp, const Message* msg) {
    if (fp == NULL || msg == NULL) {
        return CW_ERROR_NULL_PTR;
    }
    
    fprintf(fp, "%d\n", msg->type);
    fprintf(fp, "%d\n", msg->priority);
    fprintf(fp, "%d\n", msg->token_count);
    fprintf(fp, "%s\n", msg->content);
    
    return CW_SUCCESS;
}

static Message* read_message_from_file(FILE* fp) {
    if (fp == NULL) {
        return NULL;
    }
    
    int type, priority, tokens;
    char content[4096];
    
    if (fscanf(fp, "%d\n", &type) != 1) return NULL;
    if (fscanf(fp, "%d\n", &priority) != 1) return NULL;
    if (fscanf(fp, "%d\n", &tokens) != 1) return NULL;
    
    /* Read content line by line */
    if (fgets(content, sizeof(content), fp) == NULL) return NULL;
    
    /* Remove trailing newline */
    size_t len = strlen(content);
    if (len > 0 && content[len-1] == '\n') {
        content[len-1] = '\0';
    }
    
    Message* msg = (Message*)malloc(sizeof(Message));
    if (msg == NULL) return NULL;
    
    msg->content = strdup(content);
    if (msg->content == NULL) {
        free(msg);
        return NULL;
    }
    
    msg->type = (MessageType)type;
    msg->priority = (MessagePriority)priority;
    msg->token_count = tokens;
    msg->next = NULL;
    msg->prev = NULL;
    
    return msg;
}

CWResult context_window_save(const ContextWindow* window, const char* filename) {
    if (window == NULL || filename == NULL) {
        return CW_ERROR_NULL_PTR;
    }
    
    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open file for writing: %s\n", filename);
        return CW_ERROR_IO;
    }
    
    /* Write header */
    fprintf(fp, "PCC_CONTEXT_WINDOW_v1\n");
    fprintf(fp, "%d\n", window->max_tokens);
    fprintf(fp, "%d\n", window->message_count);
    
    /* Write messages */
    Message* current = window->head;
    while (current != NULL) {
        write_message_to_file(fp, current);
        current = current->next;
    }
    
    fclose(fp);
    return CW_SUCCESS;
}

ContextWindow* context_window_load(const char* filename) {
    if (filename == NULL) {
        fprintf(stderr, "Error: Filename cannot be NULL\n");
        return NULL;
    }
    
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open file for reading: %s\n", filename);
        return NULL;
    }
    
    /* Read header */
    char header[64];
    if (fgets(header, sizeof(header), fp) == NULL) {
        fclose(fp);
        return NULL;
    }
    
    /* Validate header */
    if (strncmp(header, "PCC_CONTEXT_WINDOW_v1", 21) != 0) {
        fprintf(stderr, "Error: Invalid file format\n");
        fclose(fp);
        return NULL;
    }
    
    int max_tokens, message_count;
    if (fscanf(fp, "%d\n", &max_tokens) != 1) {
        fclose(fp);
        return NULL;
    }
    if (fscanf(fp, "%d\n", &message_count) != 1) {
        fclose(fp);
        return NULL;
    }
    
    /* Create window */
    ContextWindow* window = context_window_create(max_tokens);
    if (window == NULL) {
        fclose(fp);
        return NULL;
    }
    
    /* Read messages */
    for (int i = 0; i < message_count; i++) {
        Message* msg = read_message_from_file(fp);
        if (msg == NULL) {
            break;
        }
        
        /* Add to window */
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
    }
    
    fclose(fp);
    return window;
}

CWResult context_window_export_json(const ContextWindow* window, const char* filename) {
    if (window == NULL || filename == NULL) {
        return CW_ERROR_NULL_PTR;
    }
    
    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        return CW_ERROR_IO;
    }
    
    /* Write JSON header */
    fprintf(fp, "{\n");
    fprintf(fp, "  \"version\": \"PCC_v1\",\n");
    fprintf(fp, "  \"max_tokens\": %d,\n", window->max_tokens);
    fprintf(fp, "  \"total_tokens\": %d,\n", window->total_tokens);
    fprintf(fp, "  \"message_count\": %d,\n", window->message_count);
    fprintf(fp, "  \"messages\": [\n");
    
    /* Write messages */
    Message* current = window->head;
    while (current != NULL) {
        fprintf(fp, "    {\n");
        fprintf(fp, "      \"type\": \"%s\",\n", get_message_type_string(current->type));
        fprintf(fp, "      \"priority\": \"%s\",\n", get_message_priority_string(current->priority));
        fprintf(fp, "      \"tokens\": %d,\n", current->token_count);
        
        /* Escape content for JSON */
        fprintf(fp, "      \"content\": \"");
        const char* src = current->content;
        while (*src) {
            if (*src == '"' || *src == '\\') {
                fprintf(fp, "\\");
            }
            fprintf(fp, "%c", *src);
            src++;
        }
        fprintf(fp, "\"\n    }");
        
        if (current->next != NULL) {
            fprintf(fp, ",");
        }
        fprintf(fp, "\n");
        
        current = current->next;
    }
    
    fprintf(fp, "  ]\n");
    fprintf(fp, "}\n");
    
    fclose(fp);
    return CW_SUCCESS;
}

ContextConfig context_config_default(void) {
    ContextConfig config;
    memset(&config, 0, sizeof(ContextConfig));
    
    config.max_tokens = DEFAULT_WINDOW_SIZE;
    config.min_tokens_reserve = 0;
    config.compression = COMPRESSION_LOW_PRIORITY;
    config.enable_metrics = true;
    config.thread_safe = false;
    config.token_ratio = DEFAULT_TOKEN_RATIO;
    config.auto_compress = true;
    
    return config;
}

bool context_config_validate(const ContextConfig* config) {
    if (config == NULL) {
        return false;
    }
    
    if (config->max_tokens < MIN_MAX_TOKENS || config->max_tokens > MAX_MAX_TOKENS) {
        fprintf(stderr, "Error: max_tokens must be between %d and %d\n", 
                MIN_MAX_TOKENS, MAX_MAX_TOKENS);
        return false;
    }
    
    if (config->token_ratio <= 0) {
        fprintf(stderr, "Error: token_ratio must be positive\n");
        return false;
    }
    
    if (config->min_tokens_reserve < 0 || 
        config->min_tokens_reserve >= config->max_tokens) {
        fprintf(stderr, "Error: min_tokens_reserve must be non-negative and less than max_tokens\n");
        return false;
    }
    
    return true;
}

CWResult context_window_apply_config(ContextWindow* window, const ContextConfig* config) {
    if (window == NULL || config == NULL) {
        return CW_ERROR_NULL_PTR;
    }
    
    if (!context_config_validate(config)) {
        return CW_ERROR_INVALID_PARAM;
    }
    
    /* Update configuration */
    window->config = *config;
    
    /* If max_tokens decreased, we may need to evict messages */
    if (config->max_tokens < window->max_tokens) {
        window->max_tokens = config->max_tokens;
        
        /* Trigger compression */
        if (window->config.auto_compress) {
            compress_old_messages(window);
        }
        
        /* Force eviction if still over limit */
        while (window->head != NULL && 
               window->total_tokens > window->max_tokens) {
            remove_message(window, window->head);
        }
    }
    
    return CW_SUCCESS;
}

const ContextMetrics* context_window_get_metrics(const ContextWindow* window) {
    return window ? window->metrics : NULL;
}

void context_window_reset_metrics(ContextWindow* window) {
    if (window == NULL || window->metrics == NULL) {
        return;
    }
    
    memset(window->metrics, 0, sizeof(ContextMetrics));
    window->metrics->total_time = clock();
}

void context_window_set_metrics_enabled(ContextWindow* window, bool enable) {
    if (window == NULL) {
        return;
    }
    
    if (enable && window->metrics == NULL) {
        /* Enable metrics */
        window->metrics = (ContextMetrics*)calloc(1, sizeof(ContextMetrics));
        if (window->metrics != NULL) {
            window->metrics->total_time = clock();
            window->config.enable_metrics = true;
        }
    } else if (!enable && window->metrics != NULL) {
        /* Disable metrics */
        free(window->metrics);
        window->metrics = NULL;
        window->config.enable_metrics = false;
    }
}

CWResult context_window_lock(ContextWindow* window) {
    if (window == NULL) {
        return CW_ERROR_NULL_PTR;
    }
    
    if (!window->config.thread_safe) {
        return CW_SUCCESS;  /* No-op if not thread-safe */
    }
    
    /* In production, use pthread_mutex_lock */
    /* For now, this is a placeholder */
    return CW_SUCCESS;
}

CWResult context_window_unlock(ContextWindow* window) {
    if (window == NULL) {
        return CW_ERROR_NULL_PTR;
    }
    
    if (!window->config.thread_safe) {
        return CW_SUCCESS;
    }
    
    /* In production, use pthread_mutex_unlock */
    return CW_SUCCESS;
}

bool context_window_is_thread_safe(const ContextWindow* window) {
    return window != NULL && window->config.thread_safe;
}
