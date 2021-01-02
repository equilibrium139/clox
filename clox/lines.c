#include "lines.h"

#include <assert.h>
#include <stdlib.h>

#include "memory.h"

void InitLineRunArray(LineRunArray* arr)
{
	arr->count = arr->capacity = 0;
	arr->runs = NULL;
}

void FreeLineRunArray(LineRunArray* arr)
{
	FREE_ARRAY(LineRun, arr->runs, arr->capacity);
	InitLineRunArray(arr);
}

void WriteLine(LineRunArray* arr, int line)
{
	/*assert(arr->count == 0 || arr->runs[arr->count - 1].line <= line && 
			"Lines must be written in ascending order.");*/

	if (arr->count > 0 && arr->runs[arr->count - 1].line == line)
	{
		arr->runs[arr->count - 1].count++;
		return;
	}

	if (arr->count >= arr->capacity)
	{
		int old_capacity = arr->capacity;
		arr->capacity = GROW_CAPACITY(old_capacity);
		arr->runs = GROW_ARRAY(LineRun, arr->runs, old_capacity, arr->capacity);
	}

	arr->runs[arr->count].line = line;
	arr->runs[arr->count].count = 1;
	arr->count++;
}
