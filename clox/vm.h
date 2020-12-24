#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

typedef struct
{
	Chunk* chunk;
	uint8_t* ip; // instruction pointer
	// Value stack[STACK_MAX];
	ValueArray stack;
} VM;

typedef enum {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR
} InterpretResult;

void InitVM();
void FreeVM();
InterpretResult Interpret(Chunk* chunk);
void Push(Value value);
Value Pop();

#endif
