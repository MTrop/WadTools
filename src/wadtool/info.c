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

#define ERRORINFO_NONE          0
#define ERRORINFO_NO_FILENAME   1
#define ERRORINFO_BAD_SWITCH    2
#define ERRORINFO_WAD_ERROR     10

#define SWITCH_CONDENSED        "-c"
#define SWITCH_CONDENSED2       "--condensed"

typedef struct
{
	/** WAD filename. */
	char *filename;
	/** The WAD to use. */
	wad_t *wad;
	/** If minimal info should be printed. */
	int condensed;

} wadtool_options_info_t;

static int exec(wadtool_options_info_t* options)
{
	if (options->condensed)
	{
		printf("%s ", options->filename);
		printf("%s ", options->wad->header.type == WADTYPE_IWAD ? "IWAD" : "PWAD");
		printf("%d ", options->wad->header.entry_count);
		printf("%d ", options->wad->header.entry_list_offset - sizeof(wadheader_t));
		printf("%d ", options->wad->header.entry_count * sizeof(wadentry_t));
		printf("%d ", options->wad->header.entry_list_offset);
		printf("%d\n", (options->wad->header.entry_list_offset + (sizeof(wadentry_t) * options->wad->header.entry_count)));
	}
	else
	{
		printf("%s: %s\n", options->filename, options->wad->header.type == WADTYPE_IWAD ? "IWAD" : "PWAD");
		printf("%d entries\n", options->wad->header.entry_count);
		printf("%d content bytes\n", options->wad->header.entry_list_offset - sizeof(wadheader_t));
		printf("%d list bytes\n", options->wad->header.entry_count * sizeof(wadentry_t));
		printf("list at byte %d\n", options->wad->header.entry_list_offset);
		printf("%d bytes total\n", (options->wad->header.entry_list_offset + (sizeof(wadentry_t) * options->wad->header.entry_count)));
	}
	return ERRORINFO_NONE;
}

// If nonzero, bad parse.
static int parse_file(arg_parser_t *argparser, wadtool_options_info_t *options)
{
	options->filename = currarg(argparser);
	if (!options->filename)
	{
		fprintf(stderr, "ERROR: No WAD file.\n");
		return ERRORINFO_NO_FILENAME;
	}

	// Open a shallow mapping.
	options->wad = WAD_OpenMap(options->filename);

	if (!options->wad)
	{
		if (waderrno == WADERROR_FILE_ERROR)
			fprintf(stderr, "ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
		else
			fprintf(stderr, "ERROR: %s\n", strwaderror(waderrno));
		return ERRORINFO_WAD_ERROR + waderrno;
	}
	nextarg(argparser);
	return 0;
}

// If nonzero, bad parse.
static int parse_switches(arg_parser_t *argparser, wadtool_options_info_t *options)
{
	if (matcharg(argparser, SWITCH_CONDENSED) || matcharg(argparser, SWITCH_CONDENSED2))
		options->condensed = 1;
	else if (currarg(argparser))
	{
		fprintf(stderr, "ERROR: Bad switch: %s\n", currarg(argparser));
		return ERRORINFO_BAD_SWITCH;
	}
	return 0;
}

static int call(arg_parser_t *argparser)
{
	wadtool_options_info_t options = {NULL, NULL, 0};

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
	printf("Usage: wad info [wadfile] [switches]\n");
}

static void help()
{
	printf("[wadfile]: \n");
	printf("    The name of the WAD file to inspect.\n");
	printf("\n");
	printf("[switches]: \n");
	printf("\n");
	printf("    Print options: \n");
	printf("\n");
	printf("        --condensed      Print minimal information (no headers).\n");
	printf("        -c\n");
}

wadtool_t WADTOOL_Info = {
	"info",
	"Displays basic information on a WAD file.",
	&call,
	&usage,
	&help,
};
