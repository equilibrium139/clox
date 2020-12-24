#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "lines.h"
#include "value.h"

typedef enum
{
	OP_CONSTANT,
	OP_CONSTANT_LONG,
	OP_NEGATE,
	OP_ADD,
	OP_SUB,
	OP_MULT,
	OP_DIV,
	OP_RETURN,
} OpCode;

typedef struct
{
	int count;
	int capacity;
	uint8_t* code;
	LineRunArray line_runs;	
	ValueArray constants;
} Chunk;

void InitChunk(Chunk* chunk);
void FreeChunk(Chunk* chunk);
void WriteChunk(Chunk* chunk, uint8_t value, int line);
void WriteConstant(Chunk* chunk, Value value, int line);
int AddConstant(Chunk* chunk, Value value);
int GetLine(Chunk* chunk, int instr_index);

#endif // !clox_chunk_h
