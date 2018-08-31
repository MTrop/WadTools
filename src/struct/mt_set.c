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

static int _bsearch(void **arr, int count, int (*comparefunc)(void*, void*), void *value)
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
		{
			return i;
		}
		
		c = arr[i] < value ? -1 : 1;
		
		if (c < 0)
		{
			l = i;
		}
		else
		{
			u = i;
		}
		prev = i;
		i = (u + l) / 2;
	}

	return -1;
}

// swaps contents of two addresses
static void _swap(void **a, void **b)
{
	void* temp = *a;
	*a = *b;
	*b = temp;
}

static int _expand(set_t *set, int newsize)
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

static int _canexpand(set_t *set)
{
	return set->size == set->capacity ? 1 : 0;
}

// ===========================================================================
// Public Functions
// ===========================================================================

set_t* MT_SetNew(int capacity, int (*comparefunc)(void*, void*))
{
	set_t *out = (set_t*)MTS_MALLOC(sizeof(set_t));
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

void MT_SetDestroy(set_t *set)
{
	MTS_FREE(set->items);
	MTS_FREE(set);
}

void MT_SetClear(set_t *set)
{
	set->size = 0;
}

int MT_SetLength(set_t *set)
{
	return set->size;
}

int MT_SetCapacity(set_t *set)
{
	return set->capacity;
}

int MT_SetContains(set_t *set, void *value)
{
	return _bsearch(set->items, set->size, set->comparefunc, value) >= 0 ? 1 : 0;
}

int MT_SetAdd(set_t *set, void *value)
{
	int i;
	
	if (!MT_SetContains(set, value))
	{
		if (_canexpand(set) && !_expand(set, set->capacity * 2))
			return 0;
		
		i = set->size;
		set->items[i] = value;
		(set->size)++;
		
		// insertion sort
		while (i > 0 && (*set->comparefunc)(set->items[i-1], set->items[i]) > 0)
		{
			_swap(&set->items[i-1], &set->items[i]);
			i--;
		}
		
		return 1;
	}
	return 0;
}

int MT_SetRemove(set_t *set, void *refid)
{
	int i;
	
	if ((i = _bsearch(set->items, set->size, set->comparefunc, refid)) >= 0)
	{
		// only memcpy if necessary
		if (i <= set->size - 1)
			memcpy(&set->items[i], &set->items[i+1], sizeof(void*) * (set->size - (i + 1)));

		(set->size)--;
		return 1;
	}
	return 0;
}

int MT_SetUnion(set_t *out, set_t *first, set_t *second)
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

int MT_SetIntersection(set_t *out, set_t *first, set_t *second)
{
	int i;
	int cf = first->size;
	
	MT_SetClear(out);

	for (i = 0; i < cf; i++)
		if (MT_SetContains(second, first->items[i]))
			MT_SetAdd(out, first->items[i]);

	return out->size;
}

int MT_SetXOr(set_t *out, set_t *first, set_t *second)
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

int MT_SetDifference(set_t *out, set_t *first, set_t *second)
{
	int i;
	int cf = first->size;
	
	MT_SetClear(out);

	for (i = 0; i < cf; i++)
		if (!MT_SetContains(second, first->items[i]))
			MT_SetAdd(out, first->items[i]);

	return out->size;
}

void MT_SetDump(set_t *set, void (*dumpfunc)(void*))
{
	int i;
	printf("SET CAP %d, CNT %d [ ", set->capacity, set->size);
	for (i = 0; i < set->size; i++)
	{
		if (dumpfunc)
			(*dumpfunc)(set->items[i]);
		else
			printf("%x ", set->items[i]);
	}
	printf("]\n");
}

