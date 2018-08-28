/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#ifndef __MT_HTBL_H__
#define __MT_HTBL_H__

/** Calculated hash for string hashtables. */ 
typedef int mthashvalue_t;

/** List node type. */
typedef struct HTBL_NODE
{
	/** Key object. */
	void *key;
	/** Value object. */
	void *value;
	/** Value object. */
	struct HTBL_NODE *next;
	
} HTBL_NODE;

/** Hashtable structure. */
typedef struct 
{
	/** Pointer to hashing function. */
	mthashvalue_t (*hashfunc)(void*);
	/** Pointer to key equality function. */
	int (*eqlfunc)(void*, void*);
	
	/** Pointer to table slots (and chains). */
	HTBL_NODE **table;
	/** Amount of table slots. */
	int capacity;
	/** Current amount of keys. */
	int size;
	
} mthtable_t;

/**
 * Allocates a new hashtable.
 * @param capacity the table capacity (slots).
 * @param hashfunc pointer to the hashing function to use.
 * @param eqlfunc pointer to the equality function to use.
 * @return a pointer to the table or NULL if not allocated.
 */
mthtable_t* MT_HTableCreate(int capacity, mthashvalue_t (*hashfunc)(void*), int (*eqlfunc)(void*, void*));

/**
 * Destroys a hash table.
 * DOES NOT FREE THE CONTAINED KEY AND VALUE OBJECTS.
 * @param ht the target table.
 * @return a pointer to the table or NULL if not allocated.
 */
void MT_HTableDestroy(mthtable_t *ht);

/**
 * DEBUG: Dumps the table contents.
 * @param ht the target table.
 */
void MT_HTableDump(mthtable_t *ht);

/**
 * Sets a key-value pair in the table.
 * If the key is already in the table, its corresponding value is replaced.
 * @param ht the target table.
 * @param key the key.
 * @param value the value.
 * @return 1 if successful, 0 if not.
 */
int MT_HTablePut(mthtable_t *ht, void *key, void *value);

/**
 * Returns a value in the table at the corresponding key.
 * @param ht the target table.
 * @param key the key.
 * @return the value.
 */
void* MT_HTableGet(mthtable_t *ht, void *key);

/**
 * Removes a key-value pair in the table at the corresponding key.
 * @param ht the target table.
 * @param key the key.
 * @return the corresponding value.
 */
void* MT_HTableRemove(mthtable_t *ht, void *key);

/**
 * Checks if a key is in the table.
 * @param ht the target table.
 * @param key the key.
 * @return 1 if successful, 0 if not.
 */
int MT_HTableContains(mthtable_t *ht, void *key);

/**
 * Checks if this table has no elements.
 * @param ht the target table.
 * @return 1 if so, 0 if not.
 */
int MT_HTableEmpty(mthtable_t *ht);

/**
 * @param ht the target table.
 * @return the amount of elements in the table.
 */
int MT_HTableSize(mthtable_t *ht);


/********** HELPER FUNCTIONS AND JUNK ************/

/**
 * Equality function for comparing two ints.
 */
int MT_HTableEqlInt(void* a, void* b);

/**
 * Hashing function for hashing ints.
 */
mthashvalue_t MT_HTableHashInt(void* a);

/**
 * Equality function for comparing two strings.
 */
int MT_HTableEqlCharPtr(void* a, void* b);

/**
 * Hashing function for hashing strings.
 */
mthashvalue_t MT_HTableHashCharPtr(void* a);


#endif