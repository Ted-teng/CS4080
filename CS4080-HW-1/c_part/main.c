#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node {
    char *data;
    struct Node *prev;
    struct Node *next;
} Node;

char* alloc_string(const char *str) {
    char *new_str = (char *)malloc(strlen(str) + 1);
    if (new_str) strcpy(new_str, str);
    return new_str;
}

void insert(Node **head, const char *value) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->data = alloc_string(value);
    newNode->next = NULL;
    if (*head == NULL) {
        newNode->prev = NULL;
        *head = newNode;
        return;
    }
    Node *temp = *head;
    while (temp->next != NULL) temp = temp->next;
    temp->next = newNode;
    newNode->prev = temp;
}

Node* find(Node *head, const char *value) {
    Node *temp = head;
    while (temp != NULL) {
        if (strcmp(temp->data, value) == 0) return temp;
        temp = temp->next;
    }
    return NULL;
}

void delete_node(Node **head, const char *value) {
    Node *target = find(*head, value);
    if (!target) return;
    if (target->prev) target->prev->next = target->next;
    else *head = target->next;
    if (target->next) target->next->prev = target->prev;
    free(target->data); // Free the heap-allocated string
    free(target);       // Free the node
}

int main() {
    printf("Hello, world!\n\n");
    Node *head = NULL;
    
    insert(&head, "Apple");
    insert(&head, "Banana");
    
    Node *found = find(head, "Apple");
    if (found) printf("Found: %s\n", found->data);
    
    delete_node(&head, "Apple");
    delete_node(&head, "Banana");
    return 0;
}