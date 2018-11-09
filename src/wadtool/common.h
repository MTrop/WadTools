/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#ifndef __WADTOOL_COMMON_H__
#define __WADTOOL_COMMON_H__

#include "wad/wad.h"

#define LISTFLAG_INDICES    	(1 << 0)
#define LISTFLAG_NAMES     		(1 << 1)
#define LISTFLAG_LENGTHS    	(1 << 2)
#define LISTFLAG_OFFSETS    	(1 << 3)
#define LISTFLAG_ALL        	( LISTFLAG_INDICES | LISTFLAG_NAMES | LISTFLAG_LENGTHS | LISTFLAG_OFFSETS )

/** 
 * Enum for entry type resolver. 
 */
typedef enum
{
	ET_DETECT,
	ET_INDEX,
	ET_NAME,

} entry_search_type_t;

/**
 * Single list entry for output.
 */
typedef struct 
{	
	// Entry index.
	int index;
	// Pointer to entry.
	wadentry_t *entry;

} listentry_t;

/**
 * Searches for an entry index using a string input,
 * interpreted as either numeric or string depending on entrytype.
 * @param wad the wad to search in.
 * @param entrytype the entry search type.
 * @param entry the input name/index as a string.
 * @param start the starting offset for search.
 * @return the corresponding/parsed index or -1 if not found.
 */
int WADTools_FindEntryIndex(wad_t *wad, entry_search_type_t entrytype, const char *entry, int start);

/**
 * Sort function for an array of listentry_t*.
 * See qsort(...).
 */
int WADTools_ListEntrySortIndex(const void *a, const void *b);

/**
 * Sort function for an array of listentry_t*.
 * See qsort(...).
 */
int WADTools_ListEntrySortName(const void *a, const void *b);

/**
 * Sort function for an array of listentry_t*.
 * See qsort(...).
 */
int WADTools_ListEntrySortLength(const void *a, const void *b);

/**
 * Sort function for an array of listentry_t*.
 * See qsort(...).
 */
int WADTools_ListEntrySortOffset(const void *a, const void *b);

/**
 * Prints a list of list entries to STDOUT.
 * @param entries the list of pointers to pointers to entries.
 * @param count the amount of pointers in the list.
 * @param limit the amount of entries to print.
 * @param listflags the LISTFLAG_* bits that describe what to print.
 * @param no_header if nonzero, do not print headers.
 * @param inline_header if nonzero, print headers inline (nothing printed if no_headers).
 * @param reverse if nonzero, print in reverse order.
 */
void WADTools_ListEntriesPrint(listentry_t **entries, size_t count, size_t limit, int listflags, int no_header, int inline_header, int reverse);

/**
 * Sort function for an array of listentry_t*.
 * See qsort(...).
 */
listentry_t** WADTools_ListEntryShadow(listentry_t *v, size_t n);

#endif
