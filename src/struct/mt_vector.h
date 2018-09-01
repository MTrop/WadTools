/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#ifndef __MT_VECTOR_H__
#define __MT_VECTOR_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Vector object type. */
typedef struct
{
	/** Array of objects. */
	void **items;
	/** Array capacity. */
	int capacity;
	/** Amount of objects added. */
	int size;
	
} mt_vector_t;

/**
 * Creates a dynamic vector object. 
 * This can dynamically grow in size if need be, and memory is avaliable to do so.
 * @param capacity the size in references.
 */
mt_vector_t* MT_VectorNew(int capacity);

/**
 * Frees the contents of a dynamic vector object.
 * DOES NOT FREE THE CONTAINED OBJECTS.
 * @param vector the address of the vector object.
 */
void MT_VectorDestroy(mt_vector_t *vector);

/**
 * Clears the contents of a vector object.
 * This removes all refs.
 * @param vector the address of the vector object.
 */
void MT_VectorClear(mt_vector_t *vector);

/**
 * Returns amount of refs in the selector.
 * @param vector the address of the vector object.
 */
int MT_VectorLength(mt_vector_t *vector);

/**
 * Returns amount of refs that can be added to the selector 
 * before expansion (or capacity).
 * @param vector the address of the vector object.
 */
int MT_VectorCapacity(mt_vector_t *vector);

/**
 * Changes the internal capacity of the vector.
 * If the capacity is less than the size, the size is clipped, you will lose elements in the vector.
 * @param vector the address of the vector object.
 * @param capacity the new capacity.
 */
int MT_VectorSetCapacity(mt_vector_t *vector, int capacity);

/**
 * Adds a ref (if not exist).
 * @param vector the address of the vector object.
 * @param value the reference to add.
 * @return 1 if added, 0 if not.
 */
int MT_VectorAdd(mt_vector_t *vector, void *value);

/**
 * Adds a ref at a position. 
 * All contents shift to make room for it.
 * If the position is greater than size, it is added to the end.
 * @param vector the address of the vector object.
 * @param index the target index.
 * @param value the reference to add.
 * @return 1 if added, 0 if not.
 */
int MT_VectorAddAt(mt_vector_t *vector, int index, void *value);

/**
 * Removes a ref from a position in the vector. 
 * All contents shift to fill the spot.
 * @param vector the address of the vector object.
 * @param index the target index.
 * @return the pointer to the object if removed, NULL if not.
 */
void* MT_VectorRemoveAt(mt_vector_t *vector, int index);

/**
 * Dump refs to STDOUT.
 */
void MT_VectorDump(mt_vector_t *vector, void (*dumpfunc)(void*));



#endif
