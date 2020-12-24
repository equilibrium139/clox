#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main(int argc, const char* argv[])
{
	InitVM();
	Chunk chunk;
	InitChunk(&chunk);

	WriteConstant(&chunk, 1.2, 123);
	WriteConstant(&chunk, 3.4, 123);

	WriteChunk(&chunk, OP_ADD, 123);

	WriteConstant(&chunk, 5.6, 123);

	WriteChunk(&chunk, OP_DIV, 123);
	WriteChunk(&chunk, OP_NEGATE, 123);

	WriteChunk(&chunk, OP_RETURN, 123);

	// DisassembleChunk(&chunk, "test chunk");
	Interpret(&chunk);
	FreeVM();
	FreeChunk(&chunk);
	return 0;
}