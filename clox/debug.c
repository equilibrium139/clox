#include "debug.h"

#include <stdio.h>

#include "value.h"

void DisassembleChunk(Chunk* chunk, const char* name)
{
	printf("== %s ==\n", name);
	printf("OpCode	Line	Name\n");
	for (int i = 0; i < chunk->count;)
	{
		i = DisassembleInstruction(chunk, i);
	}
}

static int SimpleInstruction(const char* name, int offset)
{
	printf("%s\n", name);
	return offset + 1;
}

static int ConstantInstruction(const char* name, Chunk* chunk, int offset)
{
	uint8_t constant_index = chunk->code[offset + 1];
	printf("%-16s %4d '", name, constant_index);
	PrintValue(chunk->constants.values[constant_index]);
	printf("'\n");
	return offset + 2;
}

static int ConstantLongInstruction(const char* name, Chunk* chunk, int offset)
{
	int constant_index = (chunk->code[offset + 1]) | (chunk->code[offset + 2] << 8) | (chunk->code[offset + 3] << 16);
	printf("%-16s %4d '", name, constant_index);
	PrintValue(chunk->constants.values[constant_index]);
	printf("'\n");
	return offset + 4;
}

// Used for instructions that are followed by indices/numbers that are not related to the constant value array

static int IndexInstruction(const char* name, Chunk* chunk, int offset)
{
	uint8_t slot = chunk->code[offset + 1];
	printf("%-16s %4d\n", name, slot);
	return offset + 2;
}

static int IndexLongInstruction(const char* name, Chunk* chunk, int offset)
{
	int index = (chunk->code[offset + 1]) | (chunk->code[offset + 2] << 8) | (chunk->code[offset + 3] << 16);
	printf("%-16s %4d\n", name, index);
	return offset + 4;
}

int DisassembleInstruction(Chunk* chunk, int offset)
{
	printf("%04d ", offset);

	int line = GetLine(chunk, offset);

	if (offset > 0 && line == GetLine(chunk, offset - 1))
	{
		printf("	|    ");
	}
	else
	{
		printf("%4d    ", line);
	}

	uint8_t instruction = chunk->code[offset];
	switch (instruction)
	{
	case OP_CONSTANT: 
		return ConstantInstruction("OP_CONSTANT", chunk, offset);
	case OP_CONSTANT_LONG:
		return ConstantLongInstruction("OP_CONSTANT_LONG", chunk, offset);
	case OP_NIL:
		return SimpleInstruction("OP_NIL", offset);
	case OP_TRUE:
		return SimpleInstruction("OP_TRUE", offset);
	case OP_FALSE:
		return SimpleInstruction("OP_FALSE", offset);
	case OP_NOT:
		return SimpleInstruction("OP_NOT", offset);
	case OP_NEGATE:
		return SimpleInstruction("OP_NEGATE", offset);
	case OP_EQUAL_SWITCH:
		return SimpleInstruction("OP_EQUAL_SWITCH", offset);
	case OP_EQUAL: 
		return SimpleInstruction("OP_EQUAL", offset);
	case OP_NOT_EQUAL:
		return SimpleInstruction("OP_NOT_EQUAL", offset);
	case OP_GREATER:
		return SimpleInstruction("OP_GREATER", offset);
	case OP_GREATER_EQUAL:
		return SimpleInstruction("OP_GREATER_EQUAL", offset);
	case OP_LESS:
		return SimpleInstruction("OP_LESS", offset);
	case OP_LESS_EQUAL:
		return SimpleInstruction("OP_LESS_EQUAL", offset);
	case OP_ADD:
		return SimpleInstruction("OP_ADD", offset);
	case OP_SUB:
		return SimpleInstruction("OP_SUB", offset);
	case OP_MULT:
		return SimpleInstruction("OP_MULT", offset);
	case OP_DIV:
		return SimpleInstruction("OP_DIV", offset);
	case OP_PRINT:
		return SimpleInstruction("OP_PRINT", offset);
	case OP_POP:
		return SimpleInstruction("OP_POP", offset);
	case OP_POPN:
		return IndexInstruction("OP_POPN", chunk, offset);
	case OP_DEFINE_GLOBAL:
		return ConstantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
	case OP_DEFINE_GLOBAL_LONG:
		return ConstantLongInstruction("OP_DEFINE_GLOBAL_LONG", chunk, offset);
	case OP_GET_GLOBAL:
		return ConstantInstruction("OP_GET_GLOBAL", chunk, offset);
	case OP_GET_GLOBAL_LONG:
		return ConstantLongInstruction("OP_GET_GLOBAL_LONG", chunk, offset);
	case OP_SET_GLOBAL:
		return ConstantInstruction("OP_SET_GLOBAL", chunk, offset);
	case OP_SET_GLOBAL_LONG:
		return ConstantLongInstruction("OP_SET_GLOBAL_LONG", chunk, offset);
	case OP_GET_LOCAL:
		return IndexInstruction("OP_GET_LOCAL", chunk, offset);
	case OP_GET_LOCAL_LONG:
		return IndexLongInstruction("OP_GET_LOCAL_LONG", chunk, offset);
	case OP_SET_LOCAL:
		return IndexInstruction("OP_SET_LOCAL", chunk, offset);
	case OP_SET_LOCAL_LONG:
		return IndexLongInstruction("OP_SET_LOCAL_LONG", chunk, offset);
	case OP_JUMP:
		return IndexLongInstruction("OP_JUMP", chunk, offset);
	case OP_JUMP_IF_FALSE:
		return IndexLongInstruction("OP_JUMP_IF_FALSE", chunk, offset);
	case OP_JUMP_IF_TRUE:
		return IndexLongInstruction("OP_JUMP_IF_TRUE", chunk, offset);
	case OP_JUMP_BACK:
		return IndexLongInstruction("OP_JUMP_BACK", chunk, offset);
	case OP_RETURN:
		return SimpleInstruction("OP_RETURN", offset);
	default:
		printf("Unknown opcode %d\n", instruction);
		return offset + 1;
	}
}