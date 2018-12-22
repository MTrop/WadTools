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
#include "io/stream.h"

extern int errno;
extern int waderrno;

#define ERRORADD_NONE				0
#define ERRORADD_NO_FILENAME		1
#define ERRORADD_MISSING_PARAMETER	2
#define ERRORADD_BAD_SWITCH			3
#define ERRORADD_WAD_ERROR			10
#define ERRORADD_IO_ERROR			20

#define SWITCH_INDEX				"-i"
#define SWITCH_INDEX2				"--index"
#define STREAMNAME_STDIN			"--"

#define STREAM_BUFFERSIZE			8192

typedef struct
{
	/** WAD filename. */
	char *filename;
	/** The WAD to use. */
	wad_t *wad;
	/** The entry name. */
	char *entryName;
	/** The source filename, or stream indicator. */
	char *source;
	/** The index to add the entry at. */
	int index;

} wadtool_options_add_t;

static void strupper(char* str)
{
	while (*str)
	{
		*str = toupper(*str);
		str++;
	}
}

static int exec(wadtool_options_add_t *options)
{
	wad_t *wad = options->wad;
	int addIndex = options->index < 0 || options->index >= WAD_EntryCount(wad) ? WAD_EntryCount(wad) : options->index;

	char newName[9];
	sprintf(newName, "%-.8s", options->entryName);

	wadentry_t *outentry;
	if (strcmp(options->source, STREAMNAME_STDIN) == 0)
		outentry = WAD_AddEntryDataAt(wad, newName, addIndex, stdin);
	else
	{
		FILE *fp = fopen(options->source, "rb");
		if (!fp)
		{
			fprintf(stderr, "ERROR: Couldn't read from %s: %s\n", options->source, strerror(errno));
			return ERRORADD_IO_ERROR + errno;
		}

		outentry = WAD_AddEntryDataAt(wad, newName, addIndex, fp);
		if (!outentry)
		{
			if (waderrno == WADERROR_FILE_ERROR)
				fprintf(stderr, "ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
			else
				fprintf(stderr, "ERROR: %s\n", strwaderror(waderrno));
			return ERRORADD_WAD_ERROR + errno;
		}

		fclose(fp);
	}

	printf("Added entry %-.8s: index %d, length %d, offset %d.\n", outentry->name, addIndex, outentry->length, outentry->offset);
	return ERRORADD_NONE;
}

// If nonzero, bad parse.
static int parse_file(arg_parser_t *argparser, wadtool_options_add_t *options)
{
	options->filename = currarg(argparser);
	if (!options->filename)
	{
		fprintf(stderr, "ERROR: No WAD file.\n");
		return ERRORADD_NO_FILENAME;
	}

	// Open a file.
	options->wad = WAD_Open(options->filename);

	if (!options->wad)
	{
		if (waderrno == WADERROR_FILE_ERROR)
			fprintf(stderr, "ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
		else
			fprintf(stderr, "ERROR: %s\n", strwaderror(waderrno));
		return ERRORADD_WAD_ERROR + waderrno;
	}
	nextarg(argparser);
	return 0;
}

#define SWITCHSTATE_INIT		0
#define SWITCHSTATE_INDEX		1

// If nonzero, bad parse.
static int parse_switches(arg_parser_t *argparser, wadtool_options_add_t *options)
{
	if (!currarg(argparser))
	{
		fprintf(stderr, "ERROR: Missing entry name.\n");
		return ERRORADD_MISSING_PARAMETER;
	}

	options->entryName = takearg(argparser);
	strupper(options->entryName);

	if (!currarg(argparser))
	{
		fprintf(stderr, "ERROR: Missing source (file name or standard stream).\n");
		return ERRORADD_MISSING_PARAMETER;
	}

	options->source = takearg(argparser);

	int state = SWITCHSTATE_INIT;
	while (currarg(argparser)) switch (state)
	{
		case SWITCHSTATE_INIT:
		{
			if (matcharg(argparser, SWITCH_INDEX) || matcharg(argparser, SWITCH_INDEX2))
				state = SWITCHSTATE_INDEX;
			else
			{
				fprintf(stderr, "ERROR: Bad switch: %s\n", currarg(argparser));
				return ERRORADD_BAD_SWITCH;
			}
		}
		break;

		case SWITCHSTATE_INDEX:
		{
			options->index = atoi(takearg(argparser));
			state = SWITCHSTATE_INIT;
		}
		break;
	}

	return 0;
}

static int call(arg_parser_t *argparser)
{
	wadtool_options_add_t options = {NULL, NULL, NULL, NULL, -1};

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
	printf("Usage: wad add [wadfile] [entryname] [source] [switches]\n");
}

static void help()
{
	printf("[wadfile]: \n");
	printf("    The name of the WAD file to add an entry to.\n");
	printf("\n");
	printf("[entryname]: \n");
	printf("    The name of the new entry.\n");
	printf("\n");
	printf("[source]: \n");
	printf("    The name of the file to add, or `--` to denote STDIN as the source.\n");
	printf("    If reading from STDIN, this will read until the end of the stream\n");
	printf("    is reached.\n");
	printf("\n");
	printf("[switches]: \n");
	printf("\n");
	printf("    Input options: \n");
	printf("\n");
	printf("        --index [index]     The list index to add the entry to (shifts the\n");
	printf("        -i [index]          other contents down a position).\n");
}

wadtool_t WADTOOL_Add = {
	"add",
	"Adds an entry to a WAD.",
	&call,
	&usage,
	&help,
};
