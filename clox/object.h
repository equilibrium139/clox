#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) AS_OBJ(value)->type
// This calls func b/c value might be a func with side effects (ex: Pop()), and value is being used more than once. We don't want to Pop twice.
#define IS_STRING(value) IsObjType(value, OBJ_STRING)
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) ((ObjString*)AS_OBJ(value))->chars

typedef enum {
	OBJ_STRING
} ObjType;

struct Obj
{
	ObjType type;
	struct Obj* next;
};

struct ObjString
{
	Obj obj;
	bool ownsChars;
	int length;
	char* chars;
	uint32_t hash;
};

static inline bool IsObjType(Value value, ObjType type)
{
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

Obj* AllocateObject(size_t size, ObjType type);
uint32_t HashString(const char* key, int length);
ObjString* CopyString(const char* str, int length);
ObjString* TakeString(char* str, int length, bool ownsChars);

#endif