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
#include <ctype.h>
#include <errno.h>
#include "wadtool.h"
#include "common.h"
#include "swap.h"
#include "wad/wad.h"
#include "wad/waderrno.h"

extern int errno;
extern int waderrno;

#define ERRORSWAP_NONE        		0
#define ERRORSWAP_NO_FILENAME 		1
#define ERRORSWAP_MISSING_PARAMETER	2
#define ERRORSWAP_BAD_ENTRY			3
#define ERRORSWAP_WAD_ERROR   		10

typedef struct
{
	/** WAD filename. */
	char *filename;
	/** The WAD to use. */
	wad_t *wad;
	/** Source entry/index. */
	char *source;
	/** Destination entry/index. */
	char *destination;

} wadtool_options_swap_t;


static void strupper(char* str)
{
	while (*str)
	{
		*str = toupper(*str);
		str++;
	}
}

static int exec(wadtool_options_swap_t *options)
{
	wad_t *wad = options->wad;

	int sidx = WADTools_FindEntryIndex(wad, ET_DETECT, options->source, 0);
	if (sidx < 0)
	{
		fprintf(stderr, "ERROR: Could not find source entry.\n");
		return ERRORSWAP_BAD_ENTRY;
	}
	else if (sidx >= WAD_EntryCount(wad))
	{
		fprintf(stderr, "ERROR: Source entry is out of range.\n");
		return ERRORSWAP_BAD_ENTRY;
	}

	int didx = WADTools_FindEntryIndex(wad, ET_DETECT, options->destination, 0);
	if (didx < 0)
	{
		fprintf(stderr, "ERROR: Could not find destination entry.\n");
		return ERRORSWAP_BAD_ENTRY;
	}
	else if (didx >= WAD_EntryCount(wad))
	{
		fprintf(stderr, "ERROR: Destination entry is out of range.\n");
		return ERRORSWAP_BAD_ENTRY;
	}

	wadentry_t *srcEntry = WAD_GetEntry(wad, sidx);
	wadentry_t *destEntry = WAD_GetEntry(wad, didx);

	if (WAD_SwapEntry(wad, sidx, didx))
	{
		if (waderrno == WADERROR_FILE_ERROR)
			fprintf(stderr, "ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
		else
			fprintf(stderr, "ERROR: %s\n", strwaderror(waderrno));
		return ERRORSWAP_WAD_ERROR + waderrno;
	}
	
	printf("Swapped index %d (%s) and %d (%s) in %s.\n", sidx, srcEntry->name, didx, destEntry->name, options->filename);

	return ERRORSWAP_NONE;
}

// If nonzero, bad parse.
static int parse_file(arg_parser_t *argparser, wadtool_options_swap_t *options)
{
	options->filename = currarg(argparser);
	if (!options->filename)
	{
		fprintf(stderr, "ERROR: No WAD file.\n");
		return ERRORSWAP_NO_FILENAME;
	}

	// Open a file.
	options->wad = WAD_Open(options->filename);

	if (!options->wad)
	{
		if (waderrno == WADERROR_FILE_ERROR)
			fprintf(stderr, "ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
		else
			fprintf(stderr, "ERROR: %s\n", strwaderror(waderrno));
		return ERRORSWAP_WAD_ERROR + waderrno;
	}
	nextarg(argparser);
	return 0;
}

// If nonzero, bad parse.
static int parse_switches(arg_parser_t *argparser, wadtool_options_swap_t *options)
{
	if (!currarg(argparser))
	{
		fprintf(stderr, "ERROR: Missing source.\n");
		return ERRORSWAP_MISSING_PARAMETER;
	}

	options->source = takearg(argparser);
	strupper(options->source);

	if (!currarg(argparser))
	{
		fprintf(stderr, "ERROR: Missing destination.\n");
		return ERRORSWAP_MISSING_PARAMETER;
	}

	options->destination = takearg(argparser);
	strupper(options->destination);

	return 0;
}

static int call(arg_parser_t *argparser)
{
	wadtool_options_swap_t options = {NULL, NULL, NULL, NULL};

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
	printf("Usage: wad swap [wadfile] [source] [destination]\n");
}

static void help()
{
	printf("[wadfile]: \n");
	printf("    The name of the WAD file to swap entries in.\n");
	printf("\n");
	printf("[source]: \n");
	printf("    The source entry index or name.\n");
	printf("\n");
	printf("[destination]: \n");
	printf("    The destination entry index or name.\n");
}

wadtool_t WADTOOL_Swap = {
	"swap",
	"Swaps the placement of two entries.",
	&call,
	&usage,
	&help,
};
