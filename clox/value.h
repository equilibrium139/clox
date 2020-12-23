#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef double Value;

typedef struct
{
	int capacity;
	int count;
	Value* values;
} ValueArray;

void InitValueArray(ValueArray* arr);
void FreeValueArray(ValueArray* arr);
void WriteValueArray(ValueArray* arr, Value value);

void PrintValue(Value value);

#endif // !clox_value_h
