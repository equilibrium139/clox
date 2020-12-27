#include "vm.h"

#include <assert.h>
#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"

VM vm;

static void ResetStack()
{
	InitValueArray(&vm.stack);
}

void InitVM()
{
	ResetStack();
}

void FreeVM()
{
	FreeValueArray(&vm.stack);
}

static InterpretResult Run()
{
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])	
// Can't use READ_BYTE() here because C++ doesn't guarantee left to right order of evaluation i.e. READ_BYTE() corresponding to READ_BYTE() << 16
// could get called before READ_BYTE() corresponding to READ_BYTE() << 8. Not good.
#define READ_CONSTANT_LONG_INDEX() (*vm.ip | (vm.ip[1] << 8) | (vm.ip[2] << 16)) 
#define READ_CONSTANT_LONG() (vm.chunk->constants.values[READ_CONSTANT_LONG_INDEX()])
#define BINARY_OP(op) \
	do { \
		assert(vm.stack.count > 1 && "Binary operator requires 2+ values on the stack."); \
		double b = Pop(); \
		vm.stack.values[vm.stack.count - 1] op= b; \
	} while (false) \

	while (true)
	{
#ifdef DEBUG_TRACE_EXECUTION
		printf("		");
		for (int i = 0; i < vm.stack.count; i++)
		{
			printf("[ ");
			PrintValue(vm.stack.values[i]);
			printf(" ]");
		}
		printf("\n");
		DisassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
		uint8_t instruction;
		switch (instruction = READ_BYTE())
		{
		case OP_CONSTANT: {
			Value constant = READ_CONSTANT();
			Push(constant);
			PrintValue(constant);
			printf("\n");
			break;
		}
		case OP_CONSTANT_LONG: {
			Value constant = READ_CONSTANT_LONG();
			Push(constant);
			vm.ip += 3;
			PrintValue(constant);
			printf("\n");
			break;
		}
		case OP_NEGATE:
		{
			vm.stack.values[vm.stack.count - 1] = -vm.stack.values[vm.stack.count - 1];
			break;
		}
		case OP_ADD:
		{
			BINARY_OP(+);
			break;
		}
		case OP_SUB:
		{
			BINARY_OP(-);
			break;
		}
		case OP_MULT:
		{
			BINARY_OP(*);
			break;
		}
		case OP_DIV:
		{
			BINARY_OP(/);
			break;
		}
		case OP_RETURN:
		{
			PrintValue(Pop());
			printf("\n");
			return INTERPRET_OK;
		}
		default:
			break;
		}
	}

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_CONSTANT_LONG_INDEX
#undef READ_CONSTANT_LONG
#undef BINARY_OP
}

InterpretResult Interpret(const char* source)
{
	Chunk chunk;
	InitChunk(&chunk);

	if (!Compile(source, &chunk))
	{
		FreeChunk(&chunk);
		return INTERPRET_COMPILE_ERROR;
	}

	vm.chunk = &chunk;
	vm.ip = chunk.code;

	InterpretResult result = Run();
	FreeChunk(&chunk);
	return result;
}

//InterpretResult Interpret(Chunk* chunk)
//{
//	vm.chunk = chunk;
//	vm.ip = chunk->code;
//	return Run();
//}

void Push(Value value)
{
	WriteValueArray(&vm.stack, value);
}

Value Pop()
{
	vm.stack.count--;
	return vm.stack.values[vm.stack.count];
}
