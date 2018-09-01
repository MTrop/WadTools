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
#include "mt_vector.h"

// ===========================================================================
// Common Private Functions
// ===========================================================================

static int _expand(mt_vector_t *vector, int newsize)
{
	void **newr;
	void **oldr = vector->items;

	newr = (void**)MTS_MALLOC(sizeof(void*) * newsize);
	if (!newr)
		return 0;

	memcpy(newr, oldr, sizeof(void*) * vector->capacity);
	vector->items = newr;
	vector->capacity = newsize;
	
	MTS_FREE(oldr);
	
	return newsize;
}

static int _canexpand(mt_vector_t *vector)
{
	return vector->size == vector->capacity ? 1 : 0;
}

// ===========================================================================
// Public Functions
// ===========================================================================

// See mt_vector.h
mt_vector_t* MT_VectorNew(int capacity)
{
	mt_vector_t *out = (mt_vector_t*)MTS_MALLOC(sizeof(mt_vector_t));
	if (!out)
	{
		return NULL;
	}
	
	out->items = (void**)MTS_MALLOC(sizeof(void*) * capacity);
	if (!out->items)
	{
		MTS_FREE(out);
		return NULL;
	}
	out->capacity = capacity;
	out->size = 0;

	return out;
}

// See mt_vector.h
void MT_VectorDestroy(mt_vector_t *vector)
{
	MTS_FREE(vector->items);
	MTS_FREE(vector);
}

// See mt_vector.h
inline void MT_VectorClear(mt_vector_t *vector)
{
	vector->size = 0;
}

// See mt_vector.h
inline int MT_VectorLength(mt_vector_t *vector)
{
	return vector->size;
}

// See mt_vector.h
inline int MT_VectorCapacity(mt_vector_t *vector)
{
	return vector->capacity;
}

// See mt_vector.h
int MT_VectorAdd(mt_vector_t *vector, void *value)
{
	int i;
	
	if (_canexpand(vector) && !_expand(vector, vector->capacity * 2))
		return 0;
	
	i = vector->size;
	vector->items[i] = value;
	(vector->size)++;
	return 1;
}


// See mt_vector.h
int MT_VectorAddAt(mt_vector_t *vector, int index, void *value)
{
	int i;

	if (_canexpand(vector) && !_expand(vector, vector->capacity * 2))
		return 0;
	
	if (index > vector->size)
	{
		MT_VectorAdd(vector, value);
		return 1;
	}
	
	for (i = vector->size; i > index; i--)
		vector->items[i] = vector->items[i - 1];

	vector->items[index] = value;
	(vector->size)++;
	return 1;
}

// See mt_vector.h
void* MT_VectorRemoveAt(mt_vector_t *vector, int index)
{
	int i;
	void *out;

	if (vector->size == 0 || index >= vector->size)
		return NULL;

	out = vector->items[index];

	if (index == vector->size - 1)
	{
		(vector->size)--;
		return out;
	}

	for (i = index; i < vector->size; i++)
		vector->items[i] = vector->items[i + 1];

	(vector->size)--;
	return out;
}

// See mt_vector.h
void MT_VectorDump(mt_vector_t *vector, void (*dumpfunc)(void*))
{
	int i;
	printf("VECTOR CAP %d, CNT %d [ ", vector->capacity, vector->size);
	for (i = 0; i < vector->size; i++)
	{
		if (dumpfunc)
			(*dumpfunc)(vector->items[i]);
		else
			printf("%x ", vector->items[i]);
	}
	printf("]\n");
}

