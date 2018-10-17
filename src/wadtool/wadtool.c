/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#include <stdlib.h>
#include "wadtool.h"

#define strieql(s,t) (stricmp((s),(t)) == 0)

int matcharg(arg_parser_t *argparser, char *t)
{
    if (strieql(argparser->arg, t))
    {
        nextarg(argparser);
        return 1;
    }
    return 0;
}

char* currarg(arg_parser_t *argparser)
{
    return argparser->arg;
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
