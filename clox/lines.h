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
	LineRun* runs; // lines[0].count instructions in 'code' correspond to line lines[0].line. See run-length encoding.
} LineRunArray;

void InitLineRunArray(LineRunArray* arr);
void FreeLineRunArray(LineRunArray* arr);
void WriteLine(LineRunArray* arr, int line);

#endif // !clox_lines_h

