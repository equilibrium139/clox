#include "object.h"

#include "memory.h"
#include "vm.h"
#include <string.h>
#include <stdio.h>

#define ALLOCATE_OBJ(type, objectType) (type*)AllocateObject(sizeof(type), objectType)

ObjFunction* NewFunction()
{
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->name = NULL;
    InitChunk(&function->chunk);

    return function;
}

ObjNative* NewNative(NativeFn function)
{
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    return native;
}

static Obj* AllocateObject(size_t size, ObjType type)
{
    Obj* object = (Obj*) reallocate(NULL, 0, size);
    object->type = type;
    object->next = vm.objects;
    vm.objects = object;
    return object;
}

static ObjString* AllocateString(char* chars, int length, uint32_t hash)
{
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->chars = chars;
    string->length = length;
    string->hash = hash;

    TableSet(&vm.strings, string, NIL_VAL);

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

    return AllocateString(heapChars, length, hash);
}

ObjString* TakeString(char* str, int length)
{
    uint32_t hash = HashString(str, length);

    ObjString* interned = TableFindString(&vm.strings, str, length, hash);
    if (interned != NULL)
    {
        FREE_ARRAY(char, str, length + 1);
        return interned;
    }

    return AllocateString(str, length, hash);
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

static void PrintFunction(ObjFunction* function)
{
    if (function->name == NULL) printf("<script>");
    else printf("<fn %s>", function->name->chars);
}

void PrintObject(Value value)
{
    switch (OBJ_TYPE(value))
    {
    case OBJ_FUNCTION:
    {
        PrintFunction(AS_FUNCTION(value));
        break;
    }
    case OBJ_STRING: {
        ObjString* str = AS_STRING(value);
        printf("%s", str->chars);
        break;
    }
    case OBJ_NATIVE:
    {
        printf("<native fn>");
        break;
    }
    default:
        break;
    }
}

