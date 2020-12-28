#ifndef clox_memory_h
#define clox_memory_h

#include "common.h"
#include "object.h"

#define GROW_CAPACITY(capacity) \
		((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, arr, old_cap, new_cap) \
		(type*)reallocate(arr, sizeof(type) * (old_cap), sizeof(type) * (new_cap))

#define FREE_ARRAY(type, arr, size) \
		reallocate(arr, sizeof(type) * (size), 0)

#define ALLOCATE(type, count) \
	(type*)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, object) \
	reallocate(object, sizeof(type), 0)

void* reallocate(void* pointer, size_t old_size, size_t new_size);
void FreeObject(Obj* obj);
void FreeObjects();

#endif // !clox_memory_h
