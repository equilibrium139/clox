#ifndef clox_object_h
#define clox_object_h

#include "chunk.h"
#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) AS_OBJ(value)->type
// This calls func b/c value might be a func with side effects (ex: Pop()), and value is being used more than once. We don't want to Pop twice.
#define IS_STRING(value) IsObjType(value, OBJ_STRING)
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) ((ObjString*)AS_OBJ(value))->chars

#define IS_FUNCTION(value) IsObjType(value, OBJ_FUNCTION)
#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))

#define IS_NATIVE(value) IsObjType(value, OBJ_NATIVE)
#define AS_NATIVE(value) (((ObjNative*)AS_OBJ(value))->function)

typedef Value(*NativeFn) (int argCount, Value* args);

typedef enum {
	OBJ_NATIVE,
	OBJ_FUNCTION,
	OBJ_STRING,
} ObjType;

struct Obj
{
	ObjType type;
	struct Obj* next;
};

typedef struct
{
	Obj obj;
	NativeFn function;
} ObjNative;

typedef struct 
{
	Obj obj;
	int arity;
	Chunk chunk;
	ObjString* name;
} ObjFunction;

struct ObjString
{
	Obj obj;
	int length;
	char* chars;
	uint32_t hash;
};

// not a macro b/c value is accessed twice and it might have side effects.
static inline bool IsObjType(Value value, ObjType type)
{
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

void PrintObject(Value value);

ObjFunction* NewFunction();
ObjNative* NewNative(NativeFn function);

Obj* AllocateObject(size_t size, ObjType type);
uint32_t HashString(const char* key, int length);
ObjString* CopyString(const char* str, int length);
ObjString* TakeString(char* str, int length);

#endif