/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mt_config.h"
#include "mt_htable.h"

// ===========================================================================
// Common Private Functions
// ===========================================================================

/**
 * Allocates memory for the hashtable.
 * @param capacity the table capacity (slots).
 */
static mthtable_t* MT_HTableAlloc(int capacity)
{
	mthtable_t *out = NULL;
	
	if (capacity <= 0)
		return NULL;
	
	out = (mthtable_t*)MTS_CALLOC(1, sizeof(mthtable_t));
	
	if (!out)
		return NULL;
	
	out->table = (HTBL_NODE**)MTS_CALLOC(capacity, sizeof(HTBL_NODE*));
	if (!out->table)
	{
		MTS_FREE(out);
		return NULL;
	}
	
	out->capacity = capacity;
	out->size = 0;
	
	return out;
}

/**
 * Returns the slot index for a key.
 * @param ht the target hashtable.
 * @param key the input key.
 */
static int MT_HTableGetSlot(mthtable_t *ht, char *key)
{
	mthashvalue_t hash = (*ht->hashfunc)(key);
	int out = (int)(hash % ht->capacity);
	return out < 0 ? -out : out;
}

/**
 * Returns the node for a key.
 * @param ht the target hashtable.
 * @param key the input key.
 * @return the corresponding hash table node.
 */
static HTBL_NODE* MT_HTableGetNode(mthtable_t *ht, char *key)
{
	HTBL_NODE *node;
	int slot = MT_HTableGetSlot(ht, key);
	
	if (!ht->table[slot])
		return NULL;
	
	node = ht->table[slot];
	while (node != NULL)
	{
		if ((*ht->eqlfunc)(key, node->key))
		{
			return node;
		}
		node = node->next;
	}
	
	return NULL;
}

// ===========================================================================
// Public Functions
// ===========================================================================

// See mt_htable.h
mthtable_t* MT_HTableCreate(int capacity, mthashvalue_t (*hashfunc)(void*), int (*eqlfunc)(void*, void*))
{
	mthtable_t *out = MT_HTableAlloc(capacity);
	if (!out)
		return NULL;
	out->hashfunc = hashfunc;
	out->eqlfunc = eqlfunc;
	return out;
}	

// See mt_htable.h
void MT_HTableDestroy(mthtable_t *ht)
{
	HTBL_NODE* current = NULL;
	HTBL_NODE* next = NULL;
	int i = 0;
	
	for (i = 0; i < ht->capacity; i++)
	{
		for (current = ht->table[i]; current != NULL; /* changed in loop */)
		{
			next = current->next;
			MTS_FREE(current);
			current = next;
		}
	}

	MTS_FREE(ht->table);
	MTS_FREE(ht);
}

// See mt_htable.h
int MT_HTablePut(mthtable_t *ht, void *key, void *value)
{
	HTBL_NODE *node, *newhead, *prev = NULL;
	int slot = MT_HTableGetSlot(ht, key);
	
	// need to make list?
	if (!ht->table[slot])
	{
		newhead = (HTBL_NODE*)MTS_CALLOC(1, sizeof(HTBL_NODE));

		if (!newhead)
			return 0;
		
		newhead->key = key;
		newhead->value = value;
		ht->table[slot] = newhead;
		ht->size = (ht->size) + 1;
		return 1;
	}
	else
	{
		node = ht->table[slot];
		while (node != NULL)
		{
			if ((*ht->eqlfunc)(key, node->key))
			{
				node->value = value;
				return 1;
			}
			prev = node;
			node = node->next;
		}
		
		prev->next = (HTBL_NODE*)MTS_CALLOC(1, sizeof(HTBL_NODE));
		node = prev->next;
		if (!node)
			return 0;

		node->key = key;
		node->value = value;
		ht->size = (ht->size) + 1;
		return 1;
	}
}

// See mt_htable.h
void* MT_HTableGet(mthtable_t *ht, void *key)
{
	HTBL_NODE *node = MT_HTableGetNode(ht, key);
	if (node)
		return node->value;
	return NULL;
}

// See mt_htable.h
void* MT_HTableRemove(mthtable_t *ht, void *key)
{
	void *out;
	HTBL_NODE *node, *prev = NULL;
	int slot = MT_HTableGetSlot(ht, key);

	node = ht->table[slot];
	while (node != NULL)
	{
		if ((*ht->eqlfunc)(key, node->key))
		{
			// first node.
			if (prev == NULL)
			{
				out = node->value;
				ht->table[slot] = node->next;
				MTS_FREE(node);
				ht->size = (ht->size) - 1;
				return out;
			}
			else
			{
				out = node->value;
				prev->next = node->next;
				MTS_FREE(node);
				ht->size = (ht->size) - 1;
				return out;
			}
		}
		prev = node;
		node = node->next;
	}
	
	return NULL;
}

// See mt_htable.h
void MT_HTableDump(mthtable_t *ht)
{
	int i;
	HTBL_NODE *current, *next;

	printf("TABLE (%x) Size %d\n", ht, ht->size);
	for (i = 0; i < ht->capacity; i++)
	{
		printf("[%d] (%x)\n", i, ht->table[i]);
		for (current = ht->table[i]; current != NULL; /* changed in loop */)
		{
			next = current->next;
			printf("\t(%x) %x -> %x\n", current, current->key, current->value);
			current = next;
		}
	}
	printf("\n");
}

// See mt_htable.h
inline int MT_HTableContains(mthtable_t *ht, void *key)
{
	return MT_HTableGetNode(ht, key) != NULL ? 1 : 0;
}

// See mt_htable.h
inline int MT_HTableEmpty(mthtable_t *ht)
{
	return ht->size > 0 ? 1 : 0;
}

// See mt_htable.h
inline int MT_HTableSize(mthtable_t *ht)
{
	return ht->size;
}

// See mt_htable.h
int MT_HTableEqlInt(void* a, void* b)
{
	return (int)a == (int)b;
}

// See mt_htable.h
mthashvalue_t MT_HTableHashInt(void* a)
{
	return (mthashvalue_t)((int)a);
}

// See mt_htable.h
int MT_HTableEqlCharPtr(void* a, void* b)
{
	return strcmp((char*)a, (char*)b) == 0;
}

// See mt_htable.h
// Adapted from the JDK's implementation of String.
mthashvalue_t MT_HTableHashCharPtr(void* a)
{
	mthashvalue_t out = 0;
	char* c = (char*)a;
	while (*c != '\0')
	{
		out = 31 * out + (*c);
		c += sizeof(char);
	}
	return out;
}


