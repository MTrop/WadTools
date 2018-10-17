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

#define ERRORCREATE_NONE 0
#define ERRORCREATE_WAD_ERROR 1
#define ERRORCREATE_NO_FILENAME 2

static int call(arg_parser_t *argparser)
{
    char* filename = currarg(argparser);
    if (!filename)
    {
        fprintf(stderr, "ERROR: No filename.\n");
        return ERRORCREATE_NO_FILENAME;
    }

	wad_t* wad = WAD_Create(filename);
	
	if (!wad)
	{
		if (waderrno == WADERROR_FILE_ERROR)
			printf("ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
		else
			printf("ERROR: %s\n", strwaderror(waderrno));
		return ERRORCREATE_WAD_ERROR;
	}
	
	WAD_Close(wad);
    
    printf("Created %s\n", filename);
    return ERRORCREATE_NONE;
}

static void usage()
{
    printf("Usage: wad create [filename]\n");
}

static void help()
{
    printf("[filename]: \n");
    printf("        The name of the WAD file to create.\n");
}

wadtool_t WADTOOL_Create = {
    "create",
    "Creates a new, empty WAD file.",
    &call,
    &usage,
    &help,
};
