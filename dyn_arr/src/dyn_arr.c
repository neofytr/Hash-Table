#include "../inc/dyn_arr.h"
#include <math.h>

dyn_arr_t *dyn_arr_create(size_t min_size, size_t item_size)
{
    if (!item_size)
    {
        return NULL;
    }

    dyn_arr_t *dyn_arr = (dyn_arr_t *)malloc(sizeof(dyn_arr_t));
    if (!dyn_arr)
    {
        return NULL;
    }

    dyn_arr->item_size = item_size;

    if (!min_size)
    {
        dyn_arr->len = 0;
        dyn_arr->nodes = NULL;
        return dyn_arr; /*  */
    }

    size_t num_of_nodes = min_size / MAX_NODE_SIZE + 1;
    void **nodes = (void **)malloc(sizeof(void *) * num_of_nodes);
    if (!nodes)
    {
        free(dyn_arr);
        return NULL;
    }

    for (size_t counter = 0; counter < num_of_nodes; counter++)
    {
        nodes[counter] = NULL;
    }

    dyn_arr->len = num_of_nodes;
    dyn_arr->nodes = nodes;
    dyn_arr->last_index = 0;

    return dyn_arr;
}

void dyn_arr_free(dyn_arr_t *dyn_arr)
{
    if (!dyn_arr)
    {
        return;
    }

    for (size_t counter = 0; counter < dyn_arr->len; counter++)
    {
        free(dyn_arr->nodes[counter]);
    }

    free(dyn_arr->nodes);
    free(dyn_arr);
}

bool dyn_arr_set(dyn_arr_t *dyn_arr, size_t index, const void *item)
{
    if (!dyn_arr || !item)
    {
        return false;
    }

    if (index > dyn_arr->last_index)
    {
        dyn_arr->last_index = index;
    }

    size_t node_index = index & (MAX_NODE_SIZE - 1);
    size_t node_no = index / MAX_NODE_SIZE;

    if (node_no >= dyn_arr->len)
    {
        // node number out of bounds, expand the node array
        size_t new_len = 1U << ((size_t)log2(node_no) + 1U);

        void *temp = realloc(dyn_arr->nodes, new_len * sizeof(void *));
        if (!temp)
        {
            return false;
        }
        dyn_arr->nodes = temp;

        // fill the resized node array with NULL pointers
        for (size_t i = dyn_arr->len; i < new_len; i++)
        {
            dyn_arr->nodes[i] = NULL;
        }

        dyn_arr->len = new_len;
    }

    if (!dyn_arr->nodes[node_no])
    {
        // if the node ptr is NULL, allocate it
        dyn_arr->nodes[node_no] = malloc(MAX_NODE_SIZE * dyn_arr->item_size);
        if (!dyn_arr->nodes[node_no])
        {
            return false;
        }
    }

    // calculate the byte offset within the node and copy the item
    void *dest = (char *)dyn_arr->nodes[node_no] + (node_index * dyn_arr->item_size);
    memcpy(dest, item, dyn_arr->item_size);
    return true;
}

bool dyn_arr_get(dyn_arr_t *dyn_arr, size_t index, void *output)
{
    if (!dyn_arr || !output)
    {
        return false;
    }

    size_t node_no = index / MAX_NODE_SIZE;
    size_t node_index = index & (MAX_NODE_SIZE - 1);

    // check if the node exists
    if (node_no >= dyn_arr->len || !dyn_arr->nodes[node_no])
    {
        return false;
    }

    // calculate the byte offset within the node and copy the item to output
    void *src = (char *)dyn_arr->nodes[node_no] + (node_index * dyn_arr->item_size);
    memcpy(output, src, dyn_arr->item_size);
    return true;
}

bool dyn_arr_max(dyn_arr_t *dyn_arr, size_t start_index, size_t end_index, dyn_compare_t is_less, void *output)
{
    if (!dyn_arr || !output || start_index > end_index)
    {
        return false;
    }

    void *max = malloc(dyn_arr->item_size);
    if (!max)
    {
        return false;
    }

    void *temp = malloc(dyn_arr->item_size);
    if (!temp)
    {
        free(max);
        return false;
    }

    if (!dyn_arr_get(dyn_arr, start_index, max))
    {
        free(max);
        free(temp);
        return false;
    }

    for (size_t counter = start_index + 1; counter <= end_index; counter++)
    {
        if (!dyn_arr_get(dyn_arr, counter, temp))
        {
            continue;
        }

        if (is_less(max, temp))
        {
            memcpy(max, temp, dyn_arr->item_size);
        }
    }

    memcpy(output, max, dyn_arr->item_size);

    free(max);
    free(temp);
    return true;
}

bool dyn_arr_min(dyn_arr_t *dyn_arr, size_t start_index, size_t end_index, dyn_compare_t is_less, void *output)
{
    if (!dyn_arr || !output || start_index > end_index)
    {
        return false;
    }

    void *min = malloc(dyn_arr->item_size);
    if (!min)
    {
        return false;
    }

    void *temp = malloc(dyn_arr->item_size);
    if (!temp)
    {
        free(min);
        return false;
    }

    if (!dyn_arr_get(dyn_arr, start_index, min))
    {
        free(min);
        free(temp);
        return false;
    }

    for (size_t counter = start_index + 1; counter <= end_index; counter++)
    {
        if (!dyn_arr_get(dyn_arr, counter, temp))
        {
            continue;
        }

        if (is_less(temp, min))
        {
            memcpy(min, temp, dyn_arr->item_size);
        }
    }

    memcpy(output, min, dyn_arr->item_size);

    free(min);
    free(temp);
    return true;
}

bool dyn_arr_sort(dyn_arr_t *dyn_arr, size_t start_index, size_t end_index, dyn_compare_t compare)
{
    if (!dyn_arr || start_index > end_index)
    {
        return false;
    }

    if (start_index == end_index)
    {
        return true;
    }

    if (start_index < end_index)
    {
        size_t mid = start_index + (end_index - start_index) / 2;

        if (!dyn_arr_sort(dyn_arr, start_index, mid, compare))
        {
            return false;
        }
        if (!dyn_arr_sort(dyn_arr, mid + 1, end_index, compare))
        {
            return false;
        }

        size_t left_len = mid - start_index + 1;
        size_t right_len = end_index - mid;

        void *left_temp = malloc(left_len * dyn_arr->item_size);
        if (!left_temp)
        {
            return false;
        }
        void *right_temp = malloc(right_len * dyn_arr->item_size);
        if (!right_temp)
        {
            free(left_temp);
            return false;
        }

        void *item_buffer = malloc(dyn_arr->item_size);
        if (!item_buffer)
        {
            free(left_temp);
            free(right_temp);
            return false;
        }

        for (size_t counter = 0; counter < left_len; counter++)
        {
            if (!dyn_arr_get(dyn_arr, start_index + counter, item_buffer))
            {
                free(left_temp);
                free(right_temp);
                free(item_buffer);
                return false;
            }
            memcpy((char *)left_temp + (counter * dyn_arr->item_size), item_buffer, dyn_arr->item_size);
        }

        for (size_t counter = 0; counter < right_len; counter++)
        {
            if (!dyn_arr_get(dyn_arr, mid + 1 + counter, item_buffer))
            {
                free(left_temp);
                free(right_temp);
                free(item_buffer);
                return false;
            }
            memcpy((char *)right_temp + (counter * dyn_arr->item_size), item_buffer, dyn_arr->item_size);
        }

        size_t left_index = 0;
        size_t right_index = 0;
        size_t main_index = start_index;

        void *left_item = malloc(dyn_arr->item_size);
        void *right_item = malloc(dyn_arr->item_size);

        if (!left_item || !right_item)
        {
            free(left_temp);
            free(right_temp);
            free(item_buffer);
            free(left_item);
            free(right_item);
            return false;
        }

        while (left_index < left_len && right_index < right_len)
        {
            memcpy(left_item, (char *)left_temp + (left_index * dyn_arr->item_size), dyn_arr->item_size);
            memcpy(right_item, (char *)right_temp + (right_index * dyn_arr->item_size), dyn_arr->item_size);

            if (compare(left_item, right_item))
            {
                if (!dyn_arr_set(dyn_arr, main_index++, left_item))
                {
                    free(left_temp);
                    free(right_temp);
                    free(item_buffer);
                    free(left_item);
                    free(right_item);
                    return false;
                }
                left_index++;
            }
            else
            {
                if (!dyn_arr_set(dyn_arr, main_index++, right_item))
                {
                    free(left_temp);
                    free(right_temp);
                    free(item_buffer);
                    free(left_temp);
                    free(right_temp);
                    free(item_buffer);
                    free(left_item);
                    free(right_item);
                    return false;
                }
                right_index++;
            }
        }

        while (left_index < left_len)
        {
            memcpy(left_item, (char *)left_temp + (left_index * dyn_arr->item_size), dyn_arr->item_size);
            if (!dyn_arr_set(dyn_arr, main_index++, left_item))
            {
                free(left_temp);
                free(right_temp);
                free(item_buffer);
                free(left_item);
                free(right_item);
                return false;
            }
            left_index++;
        }

        while (right_index < right_len)
        {
            memcpy(right_item, (char *)right_temp + (right_index * dyn_arr->item_size), dyn_arr->item_size);
            if (!dyn_arr_set(dyn_arr, main_index++, right_item))
            {
                free(left_temp);
                free(right_temp);
                free(item_buffer);
                free(left_item);
                free(right_item);
                return false;
            }
            right_index++;
        }

        free(left_temp);
        free(right_temp);
        free(item_buffer);
        free(left_item);
        free(right_item);
    }

    return true;
}

bool dyn_arr_append(dyn_arr_t *dyn_arr, const void *item)
{
    if (!dyn_arr || !item)
    {
        return false;
    }

    return dyn_arr_set(dyn_arr, dyn_arr->last_index + 1, item);
}