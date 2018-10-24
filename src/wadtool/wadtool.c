/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "wadtool.h"
#include "wad/wadconfig.h"

#define strieql(s,t) (stricmp((s),(t)) == 0)
#define stristart(s,t) (strnicmp((s),(t), strlen((s))) == 0)

void** malloc_shadow(void *v, size_t ptrsize, size_t n)
{
	int i = 0;
	void **out = (void**)WAD_MALLOC(ptrsize * n);
	if (!out)
		return NULL;
	for (i = 0; i < n; i++)
		out[i] = v + (ptrsize * i);
	return out;
}

inline char* currarg(arg_parser_t *argparser)
{
	return argparser->arg;
}

inline int currargis(arg_parser_t *argparser, char *s)
{
	return strieql(currarg(argparser), s);
}

inline int currargstart(arg_parser_t *argparser, char *s)
{
	return stristart(currarg(argparser), s);
}

char* nextarg(arg_parser_t *argparser)
{
	if (argparser->index < argparser->argc)
	{
		argparser->arg = argparser->argv[argparser->index];
		argparser->index++;
	}
	else
	{
		argparser->arg = NULL;
	}
	return argparser->arg;
}

int matcharg(arg_parser_t *argparser, char *s)
{
	if (currargis(argparser, s))
	{
		nextarg(argparser);
		return 1;
	}
	return 0;
}
