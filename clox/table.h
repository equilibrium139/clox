#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "value.h"

typedef struct
{
	ObjString* key;
	Value value;
} Entry;

typedef struct
{
	int count;
	int capacity;
	Entry* entries;
} Table;

void InitTable(Table* table);
void FreeTable(Table* table);
bool SetTable(Table* table, ObjString* key, Value value);
void TableAddAll(Table* from, Table* to);
void TableGet(Table* table, ObjString* key, Value* outValue);
bool TableDelete(Table* table, ObjString* key);
ObjString* TableFindString(Table* table, const char* chars, int length, uint32_t hash);

#endif // !clox_table_h
