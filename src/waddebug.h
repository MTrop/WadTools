/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

static void* debug_malloc(size_t size)
{
    void *ptr = malloc(size);
    printf("****MALLOC(0x%x) %d bytes\n", ptr, size);
    return ptr;
}

static void* debug_calloc(size_t n, size_t size)
{
    void *ptr = calloc(n, size);
    printf("****CALLOC(0x%x) %d bytes\n", ptr, size);
    return ptr;
}

static void* debug_realloc(void *ptr, size_t size)
{
    void *out = realloc(ptr, size);
    printf("****REALLOC(0x%x) %d bytes\n", out, size);
    return out;
}

static void debug_free(void *ptr)
{
    printf("******FREE(0x%x)\n", ptr);
    free(ptr);
}

#define WAD_MALLOC(s)		debug_malloc((s))
#define WAD_FREE(s)			debug_free((s))
#define WAD_REALLOC(p,s)	debug_realloc((p),(s))
#define WAD_CALLOC(n,s)		debug_calloc((n),(s))
