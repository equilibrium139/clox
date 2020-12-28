#include "object.h"

#include "memory.h"
#include "vm.h"
#include <string.h>

Obj* AllocateObject(size_t size, ObjType type)
{
    Obj* object = (Obj*) reallocate(NULL, 0, size);
    object->type = type;
    object->next = vm.objects;
    vm.objects = object;
    return object;
}

ObjString* ToObjString(const char* str, int length)
{
    ObjString* string = ALLOCATE_OBJ_STRING(length);
    memcpy(string->chars, str, length);
    string->chars[length] = '\0';
    string->length = length;
   /* char* strHeapCopy = ALLOCATE(char, length + 1);
    memcpy(strHeapCopy, str, length);
    strHeapCopy[length] = '\0';

    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = strHeapCopy;*/

    return string;
}
