#ifndef STACK_H
#define STACK_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct stack_node_ stack_node_t;

struct stack_node_
{
    stack_node_t *next;
    void *data;
};

typedef struct
{
    stack_node_t *bottom_node; // insertion and deletion happens at the bottom node only
    size_t data_size;
} stack_t;

stack_t *stack_create(size_t data_size); // data size in bytes
bool stack_delete(stack_t *stack);
bool stack_push(stack_t *stack, void *data);
bool is_stack_empty(stack_t *stack);
bool stack_pop(stack_t *stack, void *data); // the popped data is stored in data

#endif