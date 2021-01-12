#ifndef clox_compiler_h
#define clox_compiler_h

#include "chunk.h"
#include "object.h"

ObjFunction* Compile(const char* source);

#endif
