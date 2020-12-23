#include "chunk.h"
#include "memory.h"

void InitChunk(Chunk* chunk)
{
	chunk->count = 0;
	chunk->capacity = 0;
	chunk->code = NULL;
	InitLineRunArray(&chunk->line_runs);
	InitValueArray(&chunk->constants);
}

void FreeChunk(Chunk* chunk)
{
	FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
	FreeLineRunArray(&chunk->line_runs);
	FreeValueArray(&chunk->constants);
	InitChunk(chunk);
}

void WriteChunk(Chunk* chunk, uint8_t value, int line)
{
	if (chunk->count >= chunk->capacity)
	{
		int old_capacity = chunk->capacity;
		chunk->capacity = GROW_CAPACITY(old_capacity);
		chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_capacity, chunk->capacity);
	}

	chunk->code[chunk->count] = value;
	chunk->count++;

	WriteLine(&chunk->line_runs, line);
}

void WriteConstant(Chunk* chunk, Value value, int line)
{
	int index = chunk->constants.count;
	WriteValueArray(&chunk->constants, value);

	if (index > UINT8_MAX)
	{
		WriteChunk(chunk, OP_CONSTANT_LONG, line);
		WriteChunk(chunk, index & 0x000000FF, line);
		WriteChunk(chunk, index & 0x0000FF00, line);
		WriteChunk(chunk, index & 0x00FF0000, line);
	}
	else
	{
		WriteChunk(chunk, OP_CONSTANT, line);
		WriteChunk(chunk, index, line);
	}
}

int AddConstant(Chunk* chunk, Value value)
{
	WriteValueArray(&chunk->constants, value);
	return chunk->constants.count - 1;
}

int GetLine(Chunk* chunk, int instr_index)
{
	int line_run_index = -1;

	for (int i = instr_index + 1; i > 0; i -= chunk->line_runs.runs[line_run_index].count)
	{
		line_run_index++;
	}

	return chunk->line_runs.runs[line_run_index].line;
}
