#include "vm.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "memory.h"
#include "object.h"

VM vm;

static void ResetStack()
{
	InitValueArray(&vm.stack);
}

void InitVM()
{
	ResetStack();
	vm.objects = NULL;
	InitTable(&vm.strings);
	InitTable(&vm.globals);
}

void FreeVM()
{
	FreeTable(&vm.strings);
	FreeTable(&vm.globals);
	FreeValueArray(&vm.stack);
	FreeObjects();
}


void Push(Value value)
{
	WriteValueArray(&vm.stack, value);
}

Value Pop()
{
	vm.stack.count--;
	return vm.stack.values[vm.stack.count];
}

Value Peek(int n)
{
	return vm.stack.values[vm.stack.count - 1 - n];
}

static void RuntimeError(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);

	size_t instruction_index = vm.ip - vm.chunk->code - 1;
	int line = GetLine(vm.chunk, instruction_index);
	fprintf(stderr, "[line %d] in script\n", line);

	ResetStack();
}

static bool IsFalsey(Value value)
{
	return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static bool ValuesEqual(Value a, Value b)
{
	if (a.type != b.type)
	{
		return false;
	}

	// TYPE SWITCH
	switch (a.type)
	{
	case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
	case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
	case VAL_NIL: return true;
	case VAL_OBJ: {
		return AS_OBJ(a) == AS_OBJ(b);
		/*ObjString* aString = AS_STRING(a);
		ObjString* bString = AS_STRING(b);
		return aString->length == bString->length && memcmp(aString->chars, bString->chars, aString->length) == 0;*/
	}
	default:
		return false; // unreachable
	}
}

// Assumes v is not already OBJ_STRING
//static void StringifyNonString(Value v, char* buffer, int size)
//{
//	switch (v.type)
//	{
//	case VAL_BOOL: sprintf(buffer, AS_BOOL(v) ? "true" : "false"); break;
//
//	default:
//		break;
//	}
//	// snprintf(buffer, size, "%f")
//}

static void Concatenate()
{
	ObjString* b = AS_STRING(Pop());
	ObjString* a = AS_STRING(Pop());

	int length = a->length + b->length;
	char* concatChars = ALLOCATE(char, length + 1);
	memcpy(concatChars, a->chars, a->length);
	memcpy(concatChars + a->length, b->chars, b->length);
	concatChars[length] = '\0';

	ObjString* concatenated = TakeString(concatChars, length, true);
	Push(OBJ_VAL(concatenated));
}

static InterpretResult Run()
{
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])	
// Can't use READ_BYTE() here because C doesn't guarantee left to right order of evaluation i.e. READ_BYTE() corresponding to READ_BYTE() << 16
// could get called before READ_BYTE() corresponding to READ_BYTE() << 8. Not good.
#define READ_LONG_INDEX() (*vm.ip | (vm.ip[1] << 8) | (vm.ip[2] << 16)) 
#define READ_CONSTANT_LONG() (vm.chunk->constants.values[READ_LONG_INDEX()])
#define PEEK_TOP() (vm.stack.values[vm.stack.count - 1])
#define BINARY_OP(convertFunc, resultType, op) \
	do { \
		assert(vm.stack.count > 1 && "Binary operator requires 2+ values on the stack."); \
		if( !IS_NUMBER(Peek(0)) || !IS_NUMBER(Peek(1)) ) {\
			RuntimeError("Binary operator requires number operands."); \
			return INTERPRET_RUNTIME_ERROR; \
		} \
		double b = AS_NUMBER(Pop()); \
		convertFunc(PEEK_TOP()) = AS_NUMBER(PEEK_TOP()) op b; \
		PEEK_TOP().type = resultType; \
	} while (false) 

#define BINARY_OP_CMP(op) BINARY_OP(AS_BOOL, VAL_BOOL, op)
#define BINARY_OP_MATH(op) BINARY_OP(AS_NUMBER, VAL_NUMBER, op)
#define READ_STRING(index) AS_STRING(vm.chunk->constants.values[index])

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
		case OP_NIL: {
			Push(NIL_VAL);
			printf("NIL\n");
			break;
		}
		case OP_TRUE: {
			Push(BOOL_VAL(true));
			printf("true\n");
			break;
		}
		case OP_FALSE: {
			Push(BOOL_VAL(false));
			printf("false\n");
			break;
		}
		case OP_NOT: {
			// Everything in Lox can be converted to bool
			PEEK_TOP().as.boolean = IsFalsey(PEEK_TOP());
			PEEK_TOP().type = VAL_BOOL;
			break;
		}
		case OP_NEGATE:
		{
			if (!IS_NUMBER(Peek(0)))
			{
				RuntimeError("Negate operand must be a number.");
				return INTERPRET_RUNTIME_ERROR;
			}
			AS_NUMBER(PEEK_TOP()) = -AS_NUMBER(PEEK_TOP());
			break;
		}
		case OP_EQUAL:
		{
			Value b = Pop();
			PEEK_TOP().as.boolean = ValuesEqual(PEEK_TOP(), b);
			PEEK_TOP().type = VAL_BOOL;
			break;
		}
		case OP_NOT_EQUAL:
		{
			Value b = Pop();
			PEEK_TOP().as.boolean = !ValuesEqual(PEEK_TOP(), b);
			PEEK_TOP().type = VAL_BOOL;
			break;
		}
		case OP_GREATER:
		{
			BINARY_OP_CMP(>);
			break;
		}
		case OP_GREATER_EQUAL:
		{
			BINARY_OP_CMP(>=);
			break;
		}
		case OP_LESS:
		{
			BINARY_OP_CMP(<);
			break;
		}
		case OP_LESS_EQUAL:
		{
			BINARY_OP_CMP(<=);
			break;
		}
		case OP_ADD:
		{
			if (IS_STRING(Peek(0)) && IS_STRING(Peek(1)))
			{
				Concatenate();
			}
			else { BINARY_OP_MATH(+); }
			break;
		}
		case OP_SUB:
		{
			BINARY_OP_MATH(-);
			break;
		}
		case OP_MULT:
		{
			BINARY_OP_MATH(*);
			break;
		}
		case OP_DIV:
		{
			BINARY_OP_MATH(/);
			break;
		}
		case OP_PRINT:
		{
			PrintValue(Pop());
			printf("\n");
			break;
		}
		case OP_POP: { Pop(); break; }
		case OP_DEFINE_GLOBAL:
		{
			ObjString* name = READ_STRING(READ_BYTE());
			TableSet(&vm.globals, name, PEEK_TOP());
			Pop();
			break;
		}
		case OP_DEFINE_GLOBAL_LONG:
		{
			ObjString* name = READ_STRING(READ_LONG_INDEX());
			TableSet(&vm.globals, name, PEEK_TOP());
			Pop();
			break;
		}
		case OP_GET_GLOBAL:
		{
			ObjString* name = READ_STRING(READ_BYTE());
			Value toPush;
			if (TableGet(&vm.globals, name, &toPush))
			{
				Push(toPush);
			}
			else
			{
				RuntimeError("Undefined variable '%s'.", name->chars);
				return INTERPRET_RUNTIME_ERROR;
			}
			break;
		}
		case OP_GET_GLOBAL_LONG:
		{
			ObjString* name = READ_STRING(READ_LONG_INDEX());
			Value toPush;
			if (TableGet(&vm.globals, name, &toPush))
			{
				Push(toPush);
			}
			else
			{
				RuntimeError("Undefined variable '%s'.", name->chars);
				return INTERPRET_RUNTIME_ERROR;
			}
			break;
		}
		case OP_SET_GLOBAL:
		{
			ObjString* name = READ_STRING(READ_BYTE());
			if (TableSet(&vm.globals, name, PEEK_TOP())) 
			{
				TableDelete(&vm.globals, name);
				RuntimeError("Undefined variable '%s'.", name->chars);
				return INTERPRET_RUNTIME_ERROR;
			}
			break;
		}
		case OP_RETURN:
		{
			/*PrintValue(Pop());
			printf("\n");*/
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
#undef PEEK_TOP
#undef BINARY_OP
#undef BINARY_OP_CMP
#undef BINARY_OP_MATH
#undef READ_STRING
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
