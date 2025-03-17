#include "dyn_arr/inc/dyn_arr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

bool compare(const void *a, const void *b)
{
    uint32_t a_ = *(uint32_t *)a;
    uint32_t b_ = *(uint32_t *)b;

    return a_ < b_; // for ascending order sorting
}

int main()
{
    dyn_arr_t *arr = dyn_arr_create(0, sizeof(uint32_t));
    if (!arr)
    {
        return EXIT_FAILURE;
    }

    for (size_t index = 0; index < 100; index++)
    {
        uint32_t elt = 100 - index;
        if (!dyn_arr_append(arr, (void *)&elt))
        {
            dyn_arr_free(arr);
            return EXIT_FAILURE;
        }
    }

    if (!dyn_arr_sort(arr, 0, arr->last_index, compare))
    {
        dyn_arr_free(arr);
        return EXIT_FAILURE;
    }

    for (size_t index = 0; index < 100; index++)
    {
        uint32_t element;
        if (!dyn_arr_get(arr, index, &element))
        {
            dyn_arr_free(arr);
            return EXIT_FAILURE;
        }
        printf("%u\n", element);
    }

    dyn_arr_free(arr);
    return EXIT_SUCCESS;
}