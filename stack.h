#ifndef STACK_H
#define STACK_H

typedef struct stack_node_ stack_node_t;
typedef stack_node_t stack_t;

struct stack_node_
{
    stack_node_t *next;
    void *data;
};

#endif