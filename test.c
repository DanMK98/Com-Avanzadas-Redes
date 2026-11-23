#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int value;
    struct Node *next;
} Node;

int main(void)
{
    Node *head = malloc(sizeof *head);

    if (head == NULL)
        return 1;

    head->value = 5;
    head->next = NULL;

    printf("%d\n", head->value);
    free(head);

    return 0;
}