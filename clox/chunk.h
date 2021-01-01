#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "lines.h"
#include "value.h"

typedef enum
{
	OP_CONSTANT,
	OP_CONSTANT_LONG,
	OP_NIL,
	OP_TRUE,
	OP_FALSE,
	OP_NOT,
	OP_NEGATE,
	OP_EQUAL,
	OP_NOT_EQUAL,
	OP_GREATER,
	OP_GREATER_EQUAL,
	OP_LESS,
	OP_LESS_EQUAL,
	OP_ADD,
	OP_SUB,
	OP_MULT,
	OP_DIV,
	OP_PRINT,
	OP_POP,
	OP_POPN,
	OP_DEFINE_GLOBAL,
	OP_DEFINE_GLOBAL_LONG,
	OP_GET_GLOBAL,
	OP_GET_GLOBAL_LONG,
	OP_SET_GLOBAL,
	OP_SET_GLOBAL_LONG,
	OP_GET_LOCAL,
	OP_GET_LOCAL_LONG,
	OP_SET_LOCAL,
	OP_SET_LOCAL_LONG,
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
int WriteConstant(Chunk* chunk, Value value, int line);
int AddConstant(Chunk* chunk, Value value);
int GetLine(Chunk* chunk, int instr_index);
void WriteGlobalDeclaration(Chunk* chunk, int index, int line);
void WriteIndexOp(Chunk* chunk, int index, int line, OpCode shortOp, OpCode longOp);

#endif // !clox_chunk_h
