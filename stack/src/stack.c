#include "../inc/stack.h"

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
    if (!stack)
    {
        return false;
    }

    stack_node_t *node = stack->bottom_node;
    stack_node_t *tmp;

    while (node)
    {
        tmp = node->next;
        free(node->data);
        free(node);
        node = tmp;
    }

    free(stack);
    return true;
}

bool is_stack_empty(stack_t *stack)
{
    if (!stack)
    {
        return true;
    }
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
    memcpy(node->data, data, stack->data_size);

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

    memcpy(data, node->data, stack->data_size);

    stack->stack_size--;

    free(node->data);
    free(node);

    return true;
}
