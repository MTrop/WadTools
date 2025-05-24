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
#include "wad/wad.h"
#include "wad/waderrno.h"

extern int errno;
extern int waderrno;

#define ERRORMARKER_NONE        0
#define ERRORMARKER_NO_FILENAME 1
#define ERRORMARKER_NO_ENTRY    2
#define ERRORMARKER_WAD_ERROR   10
#define ERRORMARKER_IO_ERROR    20

typedef struct
{
	/** WAD filename. */
	char *filename;
	/** The WAD to use. */
	wad_t *wad;

	/** The name of the entry (not sanitized) */
	char *entryname;
	/** Optional index */
	int index;

} wadtool_options_marker_t;

static void strupper(char* str)
{
	while (*str)
	{
		*str = (char)toupper(*str);
		str++;
	}
}

static int exec(wadtool_options_marker_t *options)
{
	char entry[9];
	sprintf(entry, "%-.8s", options->entryname); // chop string to 8 characters

	strupper(entry);

	if (options->index < 0)
		options->index = 0;

	wadentry_t *added = WAD_AddMarkerEntryAt(options->wad, entry, options->index);

	if (!added)
	{
		if (waderrno == WADERROR_FILE_ERROR)
		{
			fprintf(stderr, "ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
			return ERRORMARKER_IO_ERROR + errno;
		}
		else
		{
			fprintf(stderr, "ERROR: %s\n", strwaderror(waderrno));
			return ERRORMARKER_WAD_ERROR + waderrno;
		}
	}

	printf("Added marker %s to index %d.\n", entry, options->index);

	return ERRORMARKER_NONE;
}

// If nonzero, bad parse.
static int parse_file(arg_parser_t *argparser, wadtool_options_marker_t *options)
{
	options->filename = currarg(argparser);
	if (!options->filename)
	{
		fprintf(stderr, "ERROR: No WAD file.\n");
		return ERRORMARKER_NO_FILENAME;
	}

	// Open a file.
	options->wad = WAD_Open(options->filename);

	if (!options->wad)
	{
		if (waderrno == WADERROR_FILE_ERROR)
		{
			fprintf(stderr, "ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
			return ERRORMARKER_IO_ERROR + errno;
		}
		else
		{
			fprintf(stderr, "ERROR: %s\n", strwaderror(waderrno));
			return ERRORMARKER_WAD_ERROR + waderrno;
		}
	}
	nextarg(argparser);
	return 0;
}

// If nonzero, bad parse.
static int parse_switches(arg_parser_t *argparser, wadtool_options_marker_t *options)
{
	options->entryname = takearg(argparser);

	if (!options->entryname)
	{
		fprintf(stderr, "ERROR: No marker entry name.");
		return ERRORMARKER_NO_ENTRY;
	}

	char *indexstr = takearg(argparser);
	if (!indexstr)
		options->index = WAD_EntryCount(options->wad);
	else
		options->index = atoi(indexstr);

	return 0;
}

static int call(arg_parser_t *argparser)
{
	wadtool_options_marker_t options = {NULL, NULL, NULL, -1};

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
	printf("Usage: wad marker [wadfile] [entry] <index>\n");
}

static void help()
{
	printf("[wadfile]: \n");
	printf("    The name of the WAD file to add a marker to.\n");
	printf("\n");
	printf("[entry]: \n");
	printf("    The name of the marker entry.\n");
	printf("\n");
	printf("<index>: Optional, default is end.\n");
	printf("    The index to add the maker entry at.\n");
}

wadtool_t WADTOOL_Marker = {
	"marker",
	"Adds a blank marker entry to a WAD file.",
	&call,
	&usage,
	&help,
};
