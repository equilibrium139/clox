#include "object.h"

#include "memory.h"
#include "vm.h"
#include <string.h>

#define ALLOCATE_OBJ(type, objectType) (type*)AllocateObject(sizeof(type), objectType)

static Obj* AllocateObject(size_t size, ObjType type)
{
    Obj* object = (Obj*) reallocate(NULL, 0, size);
    object->type = type;
    object->next = vm.objects;
    vm.objects = object;
    return object;
}

static ObjString* AllocateString(char* chars, int length, bool ownsChars, uint32_t hash)
{
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->chars = chars;
    string->length = length;
    string->ownsChars = ownsChars;
    string->hash = hash;

    SetTable(&vm.strings, string, NIL_VAL);

    return string;
}

ObjString* CopyString(const char* str, int length)
{
    uint32_t hash = HashString(str, length);
    ObjString* interned = TableFindString(&vm.strings, str, length, hash);
    if (interned != NULL) { return interned; }

    char* heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, str, length);
    heapChars[length] = '\0';

    return AllocateString(heapChars, length, true, hash);
}

ObjString* TakeString(char* str, int length, bool ownsChars)
{
    uint32_t hash = HashString(str, length);

    ObjString* interned = TableFindString(&vm.strings, str, length, hash);
    if (interned != NULL)
    {
        if (ownsChars) FREE_ARRAY(char, str, length + 1);
        return interned;
    }

    return AllocateString(str, length, ownsChars, hash);
}

uint32_t HashString(const char* key, int length)
{
    uint32_t hash = 2166136261u;
    
    for (int i = 0; i < length; i++)
    {
        hash ^= key[i];
        hash *= 16777619;
    }

    return hash;
}
