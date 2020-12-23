#ifndef clox_debug_h
#define clox_debug_h

#include "chunk.h"

void DisassembleChunk(Chunk* chunk, const char* name);
int DisassembleInstruction(Chunk* chunk, int offset);

#endif // clox_debug_h
