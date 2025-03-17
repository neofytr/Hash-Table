#include "map.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
    map_t *map = map_create(sizeof(size_t), sizeof(size_t));
    if (!map)
    {
        fprintf(stderr, "Failed to create map\n");
        return EXIT_FAILURE;
    }

    for (size_t index = 0; index < 800; index++)
    {
        map_insert(map, &index, &index);
    }

    size_t val;
    for (size_t index = 0; index < 800; index++)
    {
        if (map_search(map, &index, &val))
        {
        }
        else
        {
        }
    }

    map_destroy(map);

    return EXIT_SUCCESS;
}
