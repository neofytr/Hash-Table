#include "stack.h"

#include <string.h>

stack_t *stack_create(size_t data_size)
{
    stack_t *stack = (stack_t *)malloc(sizeof(stack_t));
    if (!stack)
    {
        return NULL;
    }

    stack->data_size = data_size;
    stack->bottom_node = NULL;
    stack->stack_size = 0;

    return stack;
}

bool stack_delete(stack_t *stack)
{
    size_t size = stack->stack_size;

    stack_node_t *node = stack->bottom_node;
    stack_node_t *tmp;

    for (size_t index = 0; index < size; index++)
    {
        tmp = node->next;
        free(node->data);
        free(node);
        node = tmp;
    }
}

bool is_stack_empty(stack_t *stack)
{
    return !stack->bottom_node;
}

bool stack_push(stack_t *stack, void *data)
{
    if (!stack || !data)
    {
        return false;
    }

    stack_node_t *node = (stack_node_t *)malloc(sizeof(stack_node_t));
    if (!node)
    {
        return false;
    }

    node->data = malloc(stack->data_size);
    if (!node->data)
    {
        free(node);
        return false;
    }

    // copy the data into the new node
    if (!memcpy(node->data, data, stack->data_size))
    {
        free(node->data);
        free(node);
        return false;
    }

    // insert the new node at the stack bottom
    node->next = stack->bottom_node;
    stack->bottom_node = node;
    stack->stack_size++;

    return true;
}

bool stack_pop(stack_t *stack, void *data)
{
    if (!stack || !data || is_stack_empty(stack))
    {
        return false;
    }

    stack_node_t *node = stack->bottom_node;
    stack->bottom_node = node->next;

    if (!memcpy(data, node->data, stack->data_size))
    {
        return false;
    }

    stack->stack_size--;

    free(node->data);
    free(node);

    return true;
}