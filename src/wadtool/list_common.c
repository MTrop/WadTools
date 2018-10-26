/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>

#include "list_common.h"
#include "wad/wad.h"
#include "wad/wad_config.h"

#define COMPARE_INT(x,y)	((x) == (y) ? 0 : ((x) < (y) ? -1 : 1))

int listentry_sort_index(const void *a, const void *b)
{
	listentry_t *x = *(listentry_t**)a;
	listentry_t *y = *(listentry_t**)b;
	return COMPARE_INT(x->index, y->index);
}

int listentry_sort_name(const void *a, const void *b)
{
	listentry_t *x = *(listentry_t**)a;
	listentry_t *y = *(listentry_t**)b;
	return strncmp(x->entry->name, y->entry->name, 8);
}

int listentry_sort_length(const void *a, const void *b)
{
	listentry_t *x = *(listentry_t**)a;
	listentry_t *y = *(listentry_t**)b;
	return COMPARE_INT(x->entry->length, y->entry->length);
}

int listentry_sort_offset(const void *a, const void *b)
{
	listentry_t *x = *(listentry_t**)a;
	listentry_t *y = *(listentry_t**)b;
	return COMPARE_INT(x->entry->offset, y->entry->offset);
}

// Print a single list entry.
static void listentry_print(listentry_t *listentry, int listflags, int no_header, int inline_header)
{
	if (!listflags || (listflags & LISTFLAG_INDICES))
	{
		if (!no_header && inline_header)
			printf("Index ");
		printf("%-10d ", listentry->index);
	}
	if (!listflags || (listflags & LISTFLAG_NAMES))
	{
		if (!no_header && inline_header)
			printf("Name ");
		printf("%-8.8s ", listentry->entry->name);
	}
	if (!listflags || (listflags & LISTFLAG_LENGTHS))
	{
		if (!no_header && inline_header)
			printf("Length ");
		printf("%-10d ", listentry->entry->length);
	}
	if (!listflags || (listflags & LISTFLAG_OFFSETS))
	{
		if (!no_header && inline_header)
			printf("Offset ");
		printf("%-10d ", listentry->entry->offset);
	}
	printf("\n");
}

void listentries_print(listentry_t **entries, size_t count, size_t limit, int listflags, int no_header, int inline_header, int reverse)
{
	if (!no_header && !inline_header)
	{
		if (!listflags || (listflags & LISTFLAG_INDICES))
			printf("Index      ");
		if (!listflags || (listflags & LISTFLAG_NAMES))
			printf("Name     ");
		if (!listflags || (listflags & LISTFLAG_LENGTHS))
			printf("Length     ");
		if (!listflags || (listflags & LISTFLAG_OFFSETS))
			printf("Offset");
		printf("\n");

		if (!listflags || (listflags & LISTFLAG_INDICES))
			printf("-----------");
		if (!listflags || (listflags & LISTFLAG_NAMES))
			printf("---------");
		if (!listflags || (listflags & LISTFLAG_LENGTHS))
			printf("-----------");
		if (!listflags || (listflags & LISTFLAG_OFFSETS))
			printf("-----------");
		printf("\n");
	}

	int i, x = 0;
	if (reverse) for (i = count - 1; i >= 0 && x < limit; i--, x++)
		listentry_print(entries[i], listflags, no_header, inline_header);
	else for (i = 0; i < count && x < limit; i++, x++)
		listentry_print(entries[i], listflags, no_header, inline_header);

	if (!no_header && !inline_header)
	{
		printf("Count %d\n", count);
	}
}

listentry_t** listentry_shadow(listentry_t *v, size_t n)
{
	int i = 0;
	listentry_t **out = (listentry_t**)WAD_MALLOC(sizeof(listentry_t*) * n);
	if (!out)
		return NULL;
	for (i = 0; i < n; i++)
		out[i] = &v[i];
	return out;
}
