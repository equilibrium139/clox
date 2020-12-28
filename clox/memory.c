#include "memory.h"

#include "vm.h"

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

void FreeObject(Obj* obj)
{
	switch (obj->type)
	{
	case OBJ_STRING: {
		ObjString* objString = (ObjString*)obj;
		FREE_ARRAY(char, objString, sizeof(ObjString) + objString->length + 1);
	}
	default:
		break;
	}
}

void FreeObjects()
{
	Obj* object = vm.objects;
	while (object != NULL)
	{
		Obj* toFree = object;
		object = object->next;
		FreeObject(toFree);
	}
}