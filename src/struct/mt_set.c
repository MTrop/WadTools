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
#include "mt_set.h"

// ===========================================================================
// Private Functions
// ===========================================================================

static int MT_DoSearch(void **arr, int count, int (*comparefunc)(void*, void*), void *value)
{
	int u, l, i, c, prev;

	u = count;
	l = 0;
	i = (u + l) / 2;
	prev = u;
	
	// binary search
	while (i != prev)
	{
		c = (*comparefunc)(arr[i], value);

		if (c == 0)
			return i;
		
		if (c < 0)
			l = i;
		else
			u = i;

		prev = i;
		i = (u + l) / 2;
	}

	return -1;
}

// swaps contents of two addresses
static void MT_SetSwap(void **a, void **b)
{
	void* temp = *a;
	*a = *b;
	*b = temp;
}

static int MT_SetExpand(mt_set_t *set, int newsize)
{
	void **newr;
	void **oldr = set->items;

	newr = (void**)MTS_MALLOC(sizeof(void*) * newsize);
	if (!newr)
		return 0;

	memcpy(newr, oldr, sizeof(void*) * set->capacity);
	set->items = newr;
	set->capacity = newsize;
	
	MTS_FREE(oldr);
	
	return newsize;
}

static int MT_SetCanExpand(mt_set_t *set)
{
	return set->size == set->capacity ? 1 : 0;
}

// ===========================================================================
// Public Functions
// ===========================================================================

mt_set_t* MT_SetCreate(int capacity, int (*comparefunc)(void*, void*))
{
	mt_set_t *out = (mt_set_t*)MTS_MALLOC(sizeof(mt_set_t));
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
	out->comparefunc = comparefunc;
	return out;
}

void MT_SetDestroy(mt_set_t *set)
{
	MTS_FREE(set->items);
	MTS_FREE(set);
}

inline void MT_SetClear(mt_set_t *set)
{
	set->size = 0;
}

inline int MT_SetLength(mt_set_t *set)
{
	return set->size;
}

inline int MT_SetCapacity(mt_set_t *set)
{
	return set->capacity;
}

inline int MT_SetSearch(mt_set_t *set, void *value)
{
	return MT_DoSearch(set->items, set->size, set->comparefunc, value);
}

int MT_SetAdd(mt_set_t *set, void *value)
{
	int i;
	
	if (!MT_SetContains(set, value))
	{
		if (MT_SetCanExpand(set) && !MT_SetExpand(set, set->capacity * 2))
			return 0;
		
		i = set->size;
		set->items[i] = value;
		(set->size)++;
		
		// insertion sort
		while (i > 0 && (*set->comparefunc)(set->items[i-1], set->items[i]) > 0)
		{
			MT_SetSwap(&set->items[i-1], &set->items[i]);
			i--;
		}
		
		return 1;
	}
	return 0;
}

void* MT_SetRemove(mt_set_t *set, void *value)
{
	int i;
	
	if ((i = MT_DoSearch(set->items, set->size, set->comparefunc, value)) >= 0)
	{
		void *out = set->items[i];
		// only memcpy if necessary
		if (i <= set->size - 1)
			memcpy(&set->items[i], &set->items[i+1], sizeof(void*) * (set->size - (i + 1)));

		(set->size)--;
		return out;
	}
	return NULL;
}

inline int MT_SetContains(mt_set_t *set, void *value)
{
	return MT_SetSearch(set, value) >= 0 ? 1 : 0;
} 

int MT_SetUnion(mt_set_t *out, mt_set_t *first, mt_set_t *second)
{
	int i;
	int cf = first->size;
	int cs = second->size;

	MT_SetClear(out);
	
	for (i = 0; i < cf; i++)
		MT_SetAdd(out, first->items[i]);
	
	for (i = 0; i < cs; i++)
		MT_SetAdd(out, second->items[i]);

	return out->size;
}

int MT_SetIntersection(mt_set_t *out, mt_set_t *first, mt_set_t *second)
{
	int i;
	int cf = first->size;
	
	MT_SetClear(out);

	for (i = 0; i < cf; i++)
		if (MT_SetContains(second, first->items[i]))
			MT_SetAdd(out, first->items[i]);

	return out->size;
}

int MT_SetXOr(mt_set_t *out, mt_set_t *first, mt_set_t *second)
{
	int i;
	int cf = first->size;
	int cs = second->size;
	
	MT_SetClear(out);

	for (i = 0; i < cf; i++)
		if (!MT_SetContains(second, first->items[i]))
			MT_SetAdd(out, first->items[i]);

	for (i = 0; i < cs; i++)
		if (!MT_SetContains(first, second->items[i]))
			MT_SetAdd(out, second->items[i]);

	return out->size;
}

int MT_SetDifference(mt_set_t *out, mt_set_t *first, mt_set_t *second)
{
	int i;
	int cf = first->size;
	
	MT_SetClear(out);

	for (i = 0; i < cf; i++)
		if (!MT_SetContains(second, first->items[i]))
			MT_SetAdd(out, first->items[i]);

	return out->size;
}

void MT_SetDump(mt_set_t *set, void (*dumpfunc)(void*))
{
	int i;
	printf("SET CAP %d, CNT %d [ ", set->capacity, set->size);
	for (i = 0; i < set->size; i++)
	{
		if (dumpfunc)
			(*dumpfunc)(set->items[i]);
		else
			printf("%x ", (unsigned int)set->items[i]);
	}
	printf("]\n");
}

