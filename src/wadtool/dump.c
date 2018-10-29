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
#include "wadio/wadstream.h"

extern int errno;
extern int waderrno;

#define ERRORDUMP_NONE        0
#define ERRORDUMP_NO_FILENAME 1
#define ERRORDUMP_WAD_ERROR   10

typedef enum
{
	ET_DETECT,
	ET_INDEX,
	ET_NAME,

} entrytype_t;

typedef struct
{
	/** WAD filename. */
	char *filename;
	/** The WAD to use. */
	wad_t *wad;

	/** Entry criteria. */
	char *criteria;
	/** Entry start criteria. */
	char *start_entry;
	/** Entry type. */
	entrytype_t entrytype;

} wadtool_options_dump_t;


static int exec(wadtool_options_dump_t *options)
{
	printf("ERROR: NOT SUPPORTED YET!\n");
	return -1;
}

#define SWITCHSTATE_INIT		0
#define SWITCHSTATE_STARTFROM	1

// If nonzero, bad parse.
static int parse_switches(arg_parser_t *argparser, wadtool_options_dump_t *options)
{
	int state = SWITCHSTATE_INIT;
	while (currarg(argparser)) switch (state)
	{
		case SWITCHSTATE_INIT:
		{

		}
		break;

		case SWITCHSTATE_STARTFROM:
		{

		}
		break;
	}

}

// If nonzero, bad parse.
static int parse_file(arg_parser_t *argparser, wadtool_options_dump_t *options)
{
	options->filename = currarg(argparser);
	if (!options->filename)
	{
		fprintf(stderr, "ERROR: No WAD file.\n");
		return ERRORDUMP_NO_FILENAME;
	}

	// Open a file.
	options->wad = WAD_Open(options->filename);

	if (!options->wad)
	{
		if (waderrno == WADERROR_FILE_ERROR)
			printf("ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
		else
			printf("ERROR: %s\n", strwaderror(waderrno));
		return ERRORDUMP_WAD_ERROR + waderrno;
	}
	nextarg(argparser);
	return 0;
}

static int call(arg_parser_t *argparser)
{
	wadtool_options_dump_t options = {NULL, NULL, NULL, NULL, ET_DETECT};

	int err;
	if (err = parse_file(argparser, &options)) // the single equals is intentional.
	{
		return err;
	}
	if (err = parse_switches(argparser, &options)) // the single equals is intentional.
	{
		WAD_Close(options.wad);
		return err;
	}
	
	int ret = exec(&options);
	WAD_Close(options.wad);
	return ret;
}

static void usage()
{
	// TODO: Finish this.
	printf("Usage: wad dump [filename] [entry] [switches]\n");
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
