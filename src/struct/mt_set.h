/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#ifndef __MT_SET_H__
#define __MT_SET_H__

/** Set object type. */
typedef struct
{
	/** Pointer to comparing function. */
	int (*comparefunc)(void*, void*);
	/** Sorted array. */
	void **items; // sorted array.
	/** Set capacity. */
	int capacity;
	/** Set length. */
	int size;
	
} mt_set_t;

/**
 * Creates a dynamic set object. 
 * This can dynamically grow in size if need be, and memory is avaliable to do so.
 * @param capacity the size in references.
 *
 * @return a pointer to the new set, or NULL if not allocated.
 */
mt_set_t* MT_SetCreate(int capacity, int (*comparefunc)(void*, void*));

/**
 * Frees the contents of a dynamic set object.
 * DOES NOT FREE THE CONTAINED OBJECTS.
 * @param set the address of the set object.
 */
void MT_SetDestroy(mt_set_t *set);

/**
 * Clears the contents of a set object.
 * This removes all refs.
 * @param set the address of the set object.
 */
void MT_SetClear(mt_set_t *set);

/**
 * Returns amount of refs in the selector.
 * @param set the address of the set object.
 */
int MT_SetLength(mt_set_t *set);

/**
 * Returns amount of refs that can be added to the selector 
 * before expansion (or capacity).
 * @param set the address of the set object.
 */
int MT_SetCapacity(mt_set_t *set);

/**
 * Adds a ref (if not exist).
 * @param set the address of the set object.
 * @param value the reference to add.
 * @return 1 if added, 0 if not.
 */
int MT_SetAdd(mt_set_t *set, void *value);

/**
 * Removes a ref (if exist).
 * @param set the set object.
 * @param value the reference to remove.
 * @return a pointer to the instance actually removed, or NULL if no match.
 */
void* MT_SetRemove(mt_set_t *set, void *value);

/**
 * Checks if a ref is in the set.
 * @param set the set object.
 * @param value the value to check for.
 * @return 1 if so, 0 if not.
 */
int MT_SetContains(mt_set_t *set, void *value);

/**
 * Finds a ref is in the set.
 * @param set the set object.
 * @param value the value to look for.
 * @return the set index (0 or greater) if found, or -1 if not.
 */
int MT_SetSearch(mt_set_t *set, void *value);

/**
 * Sets a union between two selectors.
 * The target set contents are replaced.
 * @param out the destination set.
 * @param first the first set.
 * @param second the second set.
 * @return the amount of refs in the target.
 */
int MT_SetUnion(mt_set_t *out, mt_set_t *first, mt_set_t *second);

/**
 * Sets an intersection between two selectors.
 * The target set contents are replaced.
 * @param out the destination set.
 * @param first the first set.
 * @param second the second set.
 * @return the amount of refs in the target.
 */
int MT_SetIntersection(mt_set_t *out, mt_set_t *first, mt_set_t *second);

/**
 * Sets a difference between two selectors.
 * The target set contents are replaced.
 * @param out the destination set.
 * @param first the first set.
 * @param second the second set.
 * @return the amount of refs in the target.
 */
int MT_SetDifference(mt_set_t *out, mt_set_t *first, mt_set_t *second);

/**
 * Sets an XOR between two selectors.
 * The target set contents are replaced.
 * @param out the destination set.
 * @param first the first set.
 * @param second the second set.
 * @return the amount of refs in the target.
 */
int MT_SetXOr(mt_set_t *out, mt_set_t *first, mt_set_t *second);

/**
 * Dump refs to STDOUT.
 */
void MT_SetDump(mt_set_t *set, void (*dumpfunc)(void*));

#endif