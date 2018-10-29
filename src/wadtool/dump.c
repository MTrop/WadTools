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
#include "wad/wad.h"
#include "wad/waderrno.h"

extern int errno;
extern int waderrno;

#define ERRORTEMPLATE_NONE        0
#define ERRORTEMPLATE_NO_FILENAME 1
#define ERRORTEMPLATE_WAD_ERROR   10

typedef struct
{
	/** WAD filename. */
	char *filename;
	/** The WAD to use. */
	wad_t *wad;

} wadtool_options_dump_t;


static int exec(wadtool_options_dump_t *options)
{
	printf("ERROR: NOT SUPPORTED YET!\n");
	return -1;
}

static int call(arg_parser_t *argparser)
{
	wadtool_options_dump_t options = {NULL, NULL};

	options.filename = currarg(argparser);
	if (!options.filename)
	{
		fprintf(stderr, "ERROR: No WAD file.\n");
		return ERRORTEMPLATE_NO_FILENAME;
	}

	// Open a file.
	options.wad = WAD_Open(options.filename);

	// TODO: Setup.
	
	int ret = exec(&options);
	WAD_Close(options.wad);
	return ret;
}

static void usage()
{
	// TODO: Finish this.
	printf("Usage: wad dump [filename] [entryvalue] [switches]\n");
}

static void help()
{
	printf("[filename]: \n");
	printf("    The name of the WAD file to open.\n");
	// TODO: Finish this.
}

wadtool_t WADTOOL_Dump = {
	"dump",
	"Dumps the contents of a WAD entry to STDOUT.",
	&call,
	&usage,
	&help,
};
