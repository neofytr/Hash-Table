#include "map.h"
#include <stdio.h>

int main()
{
    size_t key = 12;
    size_t value = 14;

    map_t *map = map_create(sizeof(size_t), sizeof(size_t));
    map_insert(map, &key, &value);

    value = 7;
    map_search(map, &key, &value);

    printf("%zu\n", value);

    return EXIT_SUCCESS;
}