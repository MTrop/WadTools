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

#define ERRORSHIFT_NONE        		0
#define ERRORSHIFT_NO_FILENAME 		1
#define ERRORSHIFT_NO_SOURCE 		2
#define ERRORSHIFT_NO_DESTINATION 	3
#define ERRORSHIFT_BAD_COUNT 		4
#define ERRORSHIFT_BAD_INDEX 		5
#define ERRORSHIFT_WAD_ERROR   		10

typedef struct
{
	/** WAD filename. */
	char *filename;
	/** The WAD to use. */
	wad_t *wad;

	/** Source index. */
	int source;
	/** Entry count. */
	int count;
	/** Destination index. */
	int destination;

} wadtool_options_shift_t;


static int exec(wadtool_options_shift_t *options)
{
	wad_t *wad = options->wad;

	if (options->source < 0 || options->source + options->count > WAD_EntryCount(wad))
	{
		fprintf(stderr, "ERROR: Source or source + count is out of index range.\n");
		return ERRORSHIFT_BAD_INDEX;
	}

	if (options->destination < 0 || options->destination + options->count > WAD_EntryCount(wad))
	{
		fprintf(stderr, "ERROR: Destination or destination + count is out of index range.\n");
		return ERRORSHIFT_BAD_INDEX;
	}

	if (options->count <= 0)
	{
		fprintf(stderr, "ERROR: Bad count. Must be greater than 0.\n");
		return ERRORSHIFT_BAD_COUNT;
	}

	if (WAD_ShiftEntries(wad, options->source, options->count, options->destination))
	{
		if (waderrno == WADERROR_FILE_ERROR)
			fprintf(stderr, "ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
		else
			fprintf(stderr, "ERROR: %s\n", strwaderror(waderrno));
		return ERRORSHIFT_WAD_ERROR + waderrno;
	}

	if (options->count == 1)
	{
		printf("Shifted entry %d in %s to index %d.\n", 
			options->source, 
			options->filename,
			options->destination
		);
	}
	else
	{
		printf("Shifted entries %d to %d in %s to index %d.\n", 
			options->source, 
			options->source + options->count - 1,
			options->filename,
			options->destination
		);
	}

	return ERRORSHIFT_NONE;
}

// If nonzero, bad parse.
static int parse_file(arg_parser_t *argparser, wadtool_options_shift_t *options)
{
	options->filename = currarg(argparser);
	if (!options->filename)
	{
		fprintf(stderr, "ERROR: No WAD file.\n");
		return ERRORSHIFT_NO_FILENAME;
	}

	// Open a file.
	options->wad = WAD_Open(options->filename);

	if (!options->wad)
	{
		if (waderrno == WADERROR_FILE_ERROR)
			fprintf(stderr, "ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
		else
			fprintf(stderr, "ERROR: %s\n", strwaderror(waderrno));
		return ERRORSHIFT_WAD_ERROR + waderrno;
	}
	nextarg(argparser);
	return 0;
}

// If nonzero, bad parse.
static int parse_parameters(arg_parser_t *argparser, wadtool_options_shift_t *options)
{
	if (!currarg(argparser))
	{
		fprintf(stderr, "ERROR: Expected source index.\n");
		return ERRORSHIFT_NO_SOURCE;
	}
	
	options->source = atoi(takearg(argparser));

	if (!currarg(argparser))
	{
		fprintf(stderr, "ERROR: Expected destination index.\n");
		return ERRORSHIFT_NO_DESTINATION;
	}

	options->destination = atoi(takearg(argparser));

	if (currarg(argparser))
		options->count = atoi(takearg(argparser));

	return 0;
}

static int call(arg_parser_t *argparser)
{
	wadtool_options_shift_t options = {NULL, NULL, -1, 1, -1};

	int err;
	if (err = parse_file(argparser, &options)) // the single equals is intentional.
	{
		return err;
	}
	if (err = parse_parameters(argparser, &options)) // the single equals is intentional.
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
	printf("Usage: wad shift [filename] [source] [destination] <count>\n");
}

static void help()
{
	printf("[filename]: \n");
	printf("    The name of the WAD file to shift the entries of.\n");
	printf("\n");
	printf("[source]: \n");
	printf("    The source entry index.\n");
	printf("\n");
	printf("[destination]: \n");
	printf("    The destination index.\n");
	printf("\n");
	printf("<count>: (optional, default 1)\n");
	printf("    The amount of contiguous entries from source to move.\n");
}

wadtool_t WADTOOL_Shift = {
	"shift",
	"Shifts the order of one or more entries, displacing entries.",
	&call,
	&usage,
	&help,
};
