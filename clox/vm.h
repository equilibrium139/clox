#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 64

typedef struct
{
	ObjFunction* function;
	uint8_t* ip; // function's own ip. return is handled by the vm, not by callframe
	// points to where function's locals start somewhere in vm.stack. Can't be a pointer b/c stack is resizing array. Pointers get 
	// invalidated.
	size_t slotsBeginIndex; 
} CallFrame;

typedef struct
{
	CallFrame frames[FRAMES_MAX];
	int frameCount; // # of ongoing functions
	// Chunk* chunk;
	// uint8_t* ip; // instruction pointer
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
