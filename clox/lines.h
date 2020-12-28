#ifndef clox_lines_h
#define clox_lines_h

typedef struct
{
	int line;
	int count;
} LineRun;

typedef struct
{
	int count;
	int capacity;
	// [{1, 3}, {2, 1}, {3, 4}, etc...] -> first 3 instructions correspond to line 1, next instruction to line 2, next 4 instructions to line 3, etc
	LineRun* runs; // lines[0].count instructions in 'code' correspond to line lines[0].line. See run-length encoding.
} LineRunArray;

void InitLineRunArray(LineRunArray* arr);
void FreeLineRunArray(LineRunArray* arr);
void WriteLine(LineRunArray* arr, int line);

#endif // !clox_lines_h

