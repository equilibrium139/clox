#include "object.h"

#include "memory.h"
#include "vm.h"
#include <string.h>

#define ALLOCATE_OBJ(type, objectType) \
    (type*)AllocateObject(sizeof(type), objectType)

static Obj* AllocateObject(size_t size, ObjType type)
{
    Obj* object = (Obj*) reallocate(NULL, 0, size);
    object->type = type;
    object->next = vm.objects;
    vm.objects = object;
    return object;
}

ObjString* ToObjString(const char* str, int length)
{
    char* strHeapCopy = ALLOCATE(char, length + 1);
    memcpy(strHeapCopy, str, length);
    strHeapCopy[length] = '\0';

    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = strHeapCopy;

    return string;
}
