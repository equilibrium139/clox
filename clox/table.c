#include "table.h"

#include "memory.h"
#include "object.h"
#include "value.h"

#include <string.h>

#define TABLE_MAX_LOAD 0.75

void InitTable(Table* table)
{
	table->count = 0;
	table->capacity = 0;
	table->entries = NULL;
}

void FreeTable(Table* table)
{
	FREE_ARRAY(Entry, table->entries, table->capacity);
	InitTable(table);
}

// Helper function that returns Entry e where e.key == key or e.key == NULL
static Entry* FindEntry(Entry* entries, int capacity, ObjString* key)
{
	uint32_t index = key->hash % capacity;
	Entry* entry = &entries[index];
	Entry* tombstone = NULL;

	// This loop always terminates because of the load factor (0.75 above).
	while (true)
	{		
		if (entry->key == NULL)
		{
			if (IS_NIL(entry->value))
			{
				// If we couldn't find entry e where e.key == key we return 
				return tombstone != NULL ? tombstone : entry;
			}
			else
			{
				if (tombstone == NULL) tombstone = entry;
			}
		}
		else if (entry->key == key) return entry;

		index = (index + 1) % capacity;
		entry = &entries[index];
	}
}

static void AdjustCapacity(Table* table, int capacity)
{
	Entry* adjusted = ALLOCATE(Entry, capacity);
	
	for (int i = 0; i < capacity; i++)
	{
		adjusted[i].key = NULL;
		adjusted[i].value = NIL_VAL;
	}

	Entry* old = table->entries;
	int oldCapacity = table->capacity;

	table->entries = adjusted;
	table->count = 0;
	table->capacity = capacity;

	for (int i = 0; i < oldCapacity; i++)
	{
		Entry* entry = &old[i];
		if (entry->key != NULL)
		{
			Entry* dest = FindEntry(adjusted, capacity, entry->key);
			dest->key = entry->key;
			dest->value = entry->value;
			table->count++;
		}
	}

	FREE_ARRAY(Entry, old, oldCapacity);
}

bool TableSet(Table* table, ObjString* key, Value value)
{
	if (table->count >= table->capacity * TABLE_MAX_LOAD)
	{
		int capacity = GROW_CAPACITY(table->capacity);
		AdjustCapacity(table, capacity);
	}

	Entry* entry = FindEntry(table->entries, table->capacity, key);

	bool isNewKey = entry->key == NULL;
	// new and not a tombstone. Tombstones are already included in count so no need to increment if so
	if (isNewKey && IS_NIL(entry->value)) { table->count++; }

	entry->key = key;
	entry->value = value;
	return isNewKey;
}

void TableAddAll(Table* from, Table* to)
{
	for (int i = 0; i < from->capacity; i++)
	{
		Entry* entry = &from->entries[i];
		if (entry->key != NULL)
		{
			TableSet(to, entry->key, entry->value);
		}
	}
}

bool TableGet(Table* table, ObjString* key, Value* outValue)
{
	if (table->count == 0) { return false; }

	Entry* entry = FindEntry(table->entries, table->capacity, key);
	if (entry->key != NULL) { 
		*outValue = entry->value; 
		return true;
	}

	return false;
}

bool TableDelete(Table* table, ObjString* key)
{
	if (table->count == 0) { return false; }

	Entry* entry = FindEntry(table->entries, table->capacity, key);

	if (entry->key == NULL) { return false; }

	// tombstone entry {NULL, true} or {NULL, non NIL_VAL}
	entry->key = NULL;
	entry->value = BOOL_VAL(true);

	return true;
}

ObjString* TableFindString(Table* table, const char* chars, int length, uint32_t hash)
{
	if (table->count == 0) return NULL;

	int index = hash % table->capacity;
	while (true)
	{
		Entry* entry = &table->entries[index];
		if (entry->key == NULL)
		{
			if (IS_NIL(entry->value))
			{
				return NULL;
			}
		}
		else if (entry->key->hash == hash && entry->key->length == length && memcmp(entry->key->chars, chars, length) == 0)
		{
			return entry->key;
		}

		index = (index + 1) % table->capacity;
	}
}
