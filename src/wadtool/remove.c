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
#include <ctype.h>
#include "wadtool.h"
#include "wad/wad.h"
#include "wad/waderrno.h"

extern int errno;
extern int waderrno;

#define ERRORREMOVE_NONE        0
#define ERRORREMOVE_NO_FILENAME 1
#define ERRORREMOVE_NO_INDEX    2
#define ERRORREMOVE_NO_COUNT    3
#define ERRORREMOVE_WAD_ERROR   10
#define ERRORREMOVE_IO_ERROR    20


typedef struct
{
	/** WAD filename. */
	char *filename;
	/** The WAD to use. */
	wad_t *wad;
	
	/** The entry index. */
	int index;
	/** The removal count. */
	size_t count;

} wadtool_options_remove_t;


static int exec(wadtool_options_remove_t *options)
{
	if (options->index < 0 || options->index >= WAD_EntryCount(options->wad))
	{
		fprintf(stderr, "ERROR: Index out of range.\n");
		return ERRORREMOVE_NO_INDEX;
	}

	if (options->count == 0)
	{
		fprintf(stderr, "ERROR: Nothing to remove.\n");
		return ERRORREMOVE_NO_COUNT;
	}

	if (WAD_RemoveEntryRange(options->wad, options->index, options->count))
	{
		if (waderrno == WADERROR_FILE_ERROR)
		{
			fprintf(stderr, "ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
			return ERRORREMOVE_IO_ERROR + errno;
		}
		else
		{
			fprintf(stderr, "ERROR: %s\n", strwaderror(waderrno));
			return ERRORREMOVE_WAD_ERROR + waderrno;
		}
	}

	printf("Removed %d entries from index %d.\n", options->count, options->index);
	return ERRORREMOVE_NONE;
}

// If nonzero, bad parse.
static int parse_file(arg_parser_t *argparser, wadtool_options_remove_t *options)
{
	options->filename = currarg(argparser);
	if (!options->filename)
	{
		fprintf(stderr, "ERROR: No WAD file.\n");
		return ERRORREMOVE_NO_FILENAME;
	}

	// Open a file.
	options->wad = WAD_Open(options->filename);

	if (!options->wad)
	{
		if (waderrno == WADERROR_FILE_ERROR)
		{
			fprintf(stderr, "ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
			return ERRORREMOVE_IO_ERROR + errno;
		}
		else
		{
			fprintf(stderr, "ERROR: %s\n", strwaderror(waderrno));
			return ERRORREMOVE_WAD_ERROR + waderrno;
		}
	}
	nextarg(argparser);
	return 0;
}

#define SWITCHSTATE_INIT		0
#define SWITCHSTATE_COUNT		1

// If nonzero, bad parse.
static int parse_switches(arg_parser_t *argparser, wadtool_options_remove_t *options)
{
	// Default count.
	options->count = 1;

	int state = SWITCHSTATE_INIT;
	while (currarg(argparser)) switch (state)
	{
		case SWITCHSTATE_INIT:
		{
			options->index = atoi(currarg(argparser));
			nextarg(argparser);
			state = SWITCHSTATE_COUNT;
		}
		break;

		case SWITCHSTATE_COUNT:
		{
			int count = atoi(currarg(argparser));
			if (count >= 0)
				options->count = (size_t)count;
			nextarg(argparser);
			state = SWITCHSTATE_INIT;
		}
		break;
	}

	return 0;
}

static int call(arg_parser_t *argparser)
{
	wadtool_options_remove_t options = {NULL, NULL, -1, 0};

	int err;
	if ((err = parse_file(argparser, &options)))
	{
		return err;
	}
	if ((err = parse_switches(argparser, &options)))
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
	printf("Usage: wad remove [wadfile] [entry] <count>\n");
}

static void help()
{
	printf("[wadfile]: \n");
	printf("    The WAD file to remove an entry in.\n");
	printf("\n");
	printf("[entry]: \n");
	printf("    Entry index to remove.\n");
	printf("\n");
	printf("<count>: (optional, default 1)\n");
	printf("    The amount of entries to remove, including the selected entry.\n");
}

wadtool_t WADTOOL_Remove = {
	"remove",
	"Removes one or more entries.",
	&call,
	&usage,
	&help,
};
