#include "value.h"

#include "memory.h"

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

void PrintValue(Value value)
{
	switch (value.type)
	{
	case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
	case VAL_BOOL: printf(AS_BOOL(value) ? "true" : "false"); break;
	case VAL_NIL: printf("nil"); break;
	default:
		break;
	}
}
