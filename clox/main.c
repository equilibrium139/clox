#include "common.h"
#include "chunk.h"
#include "debug.h"

int main(int argc, const char* argv[])
{
	Chunk chunk;
	InitChunk(&chunk);

	for (int i = 0; i < 1000; i++)
	{
		WriteConstant(&chunk, 1.2 * i , i);
	}

	WriteChunk(&chunk, OP_RETURN, 2000);

	DisassembleChunk(&chunk, "test chunk");
	FreeChunk(&chunk);
	return 0;
}