#include "memory.h"

#include <stdlib.h>

void* reallocate(void* arr, size_t old_size, size_t new_size)
{
    if (new_size == 0)
    {
        free(arr);
        return NULL;
    }

    void* new_arr = realloc(arr, new_size);
    if (new_arr == NULL) exit(1);
    return new_arr;
}
