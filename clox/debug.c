#include "debug.h"

#include <stdio.h>

#include "value.h"

void DisassembleChunk(Chunk* chunk, const char* name)
{
	printf("== %s ==\n", name);
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

int DisassembleInstruction(Chunk* chunk, int offset)
{
	printf("%04d ", offset);

	int line = GetLine(chunk, offset);

	if (offset > 0 && line == GetLine(chunk, offset - 1))
	{
		printf("	| ");
	}
	else
	{
		printf("%4d ", line);
	}

	uint8_t instruction = chunk->code[offset];
	switch (instruction)
	{
	case OP_CONSTANT: 
		return ConstantInstruction("OP_CONSTANT", chunk, offset);
	case OP_CONSTANT_LONG:
		return ConstantLongInstruction("OP_CONSTANT_LONG", chunk, offset);
	case OP_NEGATE:
		return SimpleInstruction("OP_NEGATE", offset);
	case OP_ADD:
		return SimpleInstruction("OP_ADD", offset);
	case OP_SUB:
		return SimpleInstruction("OP_SUB", offset);
	case OP_MULT:
		return SimpleInstruction("OP_MULT", offset);
	case OP_DIV:
		return SimpleInstruction("OP_DIV", offset);
	case OP_RETURN:
		return SimpleInstruction("OP_RETURN", offset);
	default:
		printf("Unknown opcode %d\n", instruction);
		return offset + 1;
	}
}