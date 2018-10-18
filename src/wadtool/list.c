/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "wadtool.h"
#include "../wad/wad.h"
#include "../wad/waderrno.h"

extern int errno;
extern int waderrno;

#define ERRORLIST_NONE      0

typedef struct 
{
    /** The WAD to list. */
    wad_t *wad;
    /** Minimal mode? */
    int minimal;

} wadtool_options_list_t;

static int exec(wadtool_options_list_t *options)
{
    return ERRORLIST_NONE;
}

static int call(arg_parser_t *argparser)
{

    // TODO: Finish this.
    return ERRORLIST_NONE;
}

static void usage()
{
    // TODO: Finish this.
    printf("Usage: asdfasdfasdf\n");
}

static void help()
{
    // TODO: Finish this.
}

wadtool_t WADTOOL_List = {
    "list",
    "Lists the entries in a WAD.",
    &call,
    &usage,
    &help,
};
