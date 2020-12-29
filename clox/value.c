#include "value.h"

#include "memory.h"
#include "object.h"

void InitValueArray(ValueArray* arr)
{
	arr->count = arr->capacity = 0;
	arr->values = NULL;
}

void FreeValueArray(ValueArray* arr)
{
	FREE_ARRAY(Value, arr->values, arr->capacity);
	InitValueArray(arr);
}

void WriteValueArray(ValueArray* arr, Value value)
{
	if (arr->count >= arr->capacity)
	{
		int old_capacity = arr->capacity;
		arr->capacity = GROW_CAPACITY(old_capacity);
		arr->values = GROW_ARRAY(Value, arr->values, old_capacity, arr->capacity);
	}

	arr->values[arr->count] = value;
	arr->count++;
}

void PrintObject(Value value)
{
	switch (OBJ_TYPE(value))
	{
	case OBJ_STRING: {
		ObjString* str = AS_STRING(value);
		if (str->ownsChars) printf("%s", str->chars);
		else printf("%.*s", str->length, str->chars); 
		break;
	}
	default:
		break;
	}
}

void PrintValue(Value value)
{
	switch (value.type)
	{
	case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
	case VAL_BOOL: printf(AS_BOOL(value) ? "true" : "false"); break;
	case VAL_NIL: printf("nil"); break;
	case VAL_OBJ: PrintObject(value); break;
	default:
		break;
	}
}
