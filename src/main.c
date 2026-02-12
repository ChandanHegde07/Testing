#include "context_window.h"

int main() {
    printf("SLM Context Window Manager\n");
    printf("==========================\n\n");
    
    // Create a context window with 1000 token limit
    ContextWindow* window = context_window_create(1000);
    if (window == NULL) {
        printf("Failed to create context window\n");
        return 1;
    }
    
    // Add some example messages
    printf("Adding initial messages...\n\n");
    
    context_window_add_message(window, MESSAGE_SYSTEM, PRIORITY_CRITICAL, 
        "You are a helpful AI assistant. Please answer questions concisely.");
    
    context_window_add_message(window, MESSAGE_USER, PRIORITY_HIGH, 
        "Hello! Can you help me with a programming problem?");
    
    context_window_add_message(window, MESSAGE_ASSISTANT, PRIORITY_NORMAL, 
        "Of course! What do you need help with?");
    
    context_window_add_message(window, MESSAGE_USER, PRIORITY_HIGH, 
        "I'm trying to implement a linked list in C. How should I start?");
    
    context_window_add_message(window, MESSAGE_ASSISTANT, PRIORITY_NORMAL, 
        "Great question! Let's start by defining the basic structure:\n"
        "\n"
        "typedef struct Node {\n"
        "    int data;\n"
        "    struct Node* next;\n"
        "} Node;\n"
        "\n"
        "Then you'll need functions to create nodes, add them to the list, "
        "remove them, and traverse the list.");
    
    context_window_add_message(window, MESSAGE_USER, PRIORITY_NORMAL, 
        "Thanks! That makes sense. What about memory management?");
    
    context_window_add_message(window, MESSAGE_ASSISTANT, PRIORITY_NORMAL, 
        "Memory management is crucial. Always remember to free the memory "
        "when you're done with the linked list. Here's how to free all nodes:\n"
        "\n"
        "void free_list(Node* head) {\n"
        "    Node* temp;\n"
        "    while (head != NULL) {\n"
        "        temp = head;\n"
        "        head = head->next;\n"
        "        free(temp);\n"
        "    }\n"
        "}\n"
        "\n"
        "Also, make sure to check if malloc() returns NULL to handle out-of-memory errors.");
    
    // Print context window statistics
    context_window_print_stats(window);
    printf("\n");
    
    // Get and print the optimized context
    printf("Optimized Context for SLM API:\n");
    printf("-------------------------------\n");
    char* context = context_window_get_context(window);
    if (context != NULL) {
        printf("%s", context);
        free(context);
    }
    
    // Clean up
    context_window_destroy(window);
    
    printf("\n==========================\n");
    printf("Context window manager demo completed.\n");
    
    return 0;
}
