#include "map.h"
#include <stdio.h>

int main()
{
    size_t value;

    map_t *map = map_create(sizeof(size_t), sizeof(size_t));

    for (size_t index = 0; index < 1024; index++)
    {
        value = index;
        map_insert(map, &index, &value);
    }

    for (size_t index = 0; index < 1024; index++)
    {
        map_search(map, &index, &value);
        printf("%zu\n", value);
    }

    return EXIT_SUCCESS;
}