#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "table.h"
#include "value.h"

#define STACK_MAX 256

typedef struct
{
	Chunk* chunk;
	uint8_t* ip; // instruction pointer
	// Value stack[STACK_MAX];
	ValueArray stack;
	Table strings;
	Table globals;
	Obj* objects;
} VM;

typedef enum {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void InitVM();
void FreeVM();
InterpretResult Interpret(const char* source);
void Push(Value value);
Value Pop();

#endif
