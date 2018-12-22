/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#define PATHSEPARATOR '\\'
#endif
#ifndef _WIN32
#define PATHSEPARATOR '/'
#endif
#define EXTENSIONSEPARATOR '.'

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
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
#define SWITCH_ENTRYNAME			"-n"
#define SWITCH_ENTRYNAME2			"--entry-name"
#define SWITCH_LIST					"-l"
#define SWITCH_LIST2				"--list"
#define STREAMNAME_STDIN			"--"

#define STREAM_BUFFERSIZE			8192

#define MIN(a,b) ((a) < (b) ? (a) : (b))

typedef struct
{
	/** WAD filename. */
	char *filename;
	/** The WAD to use. */
	wad_t *wad;
	/** The specified entry name. */
	char *entryName;
	/** The source filename, or stream indicator. */
	char *source;
	/** If nonzero, the source is a list of files. */
	int sourceIsList;
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

static void extractFileName(char *targetBuffer, char *filename, size_t maxbytes)
{
	char *nameptr, *endptr;
	nameptr = (nameptr = strrchr(filename, PATHSEPARATOR)) ? ++nameptr : filename;
	endptr = (endptr = strchr(nameptr, EXTENSIONSEPARATOR)) ? endptr : nameptr + strlen(nameptr);
	size_t len = MIN(endptr - nameptr, maxbytes);
	memcpy(targetBuffer, nameptr, len);
	if (len < maxbytes)
		*(targetBuffer+len) = '\0';
}

static int add(wad_t *wad, char *sourceFile, char *entryName, int addIndex)
{
	char newName[9];
	sprintf(newName, "%-.8s", entryName);
	strupper(newName);

	wadentry_t *outentry;
	if (strcmp(sourceFile, STREAMNAME_STDIN) == 0)
	{
		#ifdef _WIN32
		_setmode(_fileno(stdin), _O_BINARY);
		#endif

		outentry = WAD_AddEntryDataAt(wad, newName, addIndex, stdin);

		#ifdef _WIN32
		_setmode(_fileno(stdin), _O_TEXT);
		#endif
	}
	else
	{
		FILE *fp = fopen(sourceFile, "rb");
		if (!fp)
		{
			fprintf(stderr, "ERROR: Couldn't read from %s: %s\n", sourceFile, strerror(errno));
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

static int exec(wadtool_options_add_t *options)
{
	wad_t *wad = options->wad;
	int addIndex = options->index < 0 || options->index >= WAD_EntryCount(wad) ? WAD_EntryCount(wad) : options->index;

	char entryName[512];
	char *sourceFile = options->source;
	if (options->sourceIsList)
	{
		stream_t *listin;

		if (strcmp(sourceFile, STREAMNAME_STDIN) == 0)
		{
			listin = STREAM_OpenFile(stdin);
			if (!listin)
			{
				fprintf(stderr, "ERROR: Couldn't read from STDIN!\n");
				return ERRORADD_IO_ERROR;
			}
		}
		else
		{
			listin = STREAM_Open(sourceFile);
			if (!listin)
			{
				fprintf(stderr, "ERROR: Couldn't read from %s: %s\n", sourceFile, strerror(errno));
				return ERRORADD_IO_ERROR + errno;
			}
		}

		int ret = 0;
		char filenameLine[512];
		while (!ret && STREAM_ReadLine(listin, filenameLine, 512) >= 0)
		{
			if (options->entryName)
				ret = add(wad, filenameLine, options->entryName, addIndex++);
			else
			{
				extractFileName(entryName, filenameLine, 512);
				ret = add(wad, filenameLine, entryName, addIndex++);
			}
		}

		STREAM_Close(listin);
		return ret;
	}
	else
	{
		int ret = 0;
		if (options->entryName)
			ret = add(wad, sourceFile, options->entryName, addIndex++);
		else
		{
			if (strcmp(sourceFile, STREAMNAME_STDIN) == 0)
			{
				fprintf(stderr, "ERROR: Must specify an entry name via `--entry-name` switch if source is STDIN.\n");
				return ERRORADD_MISSING_PARAMETER;
			}
			extractFileName(entryName, sourceFile, 512);
			ret = add(wad, sourceFile, entryName, addIndex++);
		}
		return ret;
	}
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
#define SWITCHSTATE_ENTRYNAME	2

// If nonzero, bad parse.
static int parse_switches(arg_parser_t *argparser, wadtool_options_add_t *options)
{
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
			else if (matcharg(argparser, SWITCH_ENTRYNAME) || matcharg(argparser, SWITCH_ENTRYNAME2))
				state = SWITCHSTATE_ENTRYNAME;
			else if (matcharg(argparser, SWITCH_LIST) || matcharg(argparser, SWITCH_LIST2))
				options->sourceIsList = 1;
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

		case SWITCHSTATE_ENTRYNAME:
		{
			options->entryName = takearg(argparser);
			state = SWITCHSTATE_INIT;
		}
		break;
	}

	if (state == SWITCHSTATE_INDEX)
	{
		fprintf(stderr, "ERROR: Missing index number after switch.\n");
		return ERRORADD_MISSING_PARAMETER;
	}

	if (state == SWITCHSTATE_ENTRYNAME)
	{
		fprintf(stderr, "ERROR: Missing entry name after switch.\n");
		return ERRORADD_MISSING_PARAMETER;
	}

	return 0;
}

static int call(arg_parser_t *argparser)
{
	wadtool_options_add_t options = {NULL, NULL, NULL, NULL, 0, -1};

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
	printf("Usage: wad add [wadfile] [source] [switches]\n");
}

static void help()
{
	printf("[wadfile]: \n");
	printf("    The name of the WAD file to add an entry to.\n");
	printf("\n");
	printf("[source]: \n");
	printf("    The name of the file to add, or `--` to denote STDIN as the source.\n");
	printf("    If reading from STDIN, this will read until the end of the stream\n");
	printf("    is reached, and the `--entry-name` switch MUST be used to specify an\n");
	printf("    entry name.\n");
	printf("\n");
	printf("[switches]: \n");
	printf("\n");
	printf("    Input options: \n");
	printf("\n");
	printf("        --entry-name [entryname]  Specifies the name of the new entry. If\n");
	printf("        -n [entryname]            not provided, it is created from the\n");
	printf("                                  input filename (REQUIRED if reading from\n");
	printf("                                  STDIN).\n");
	printf("\n");
	printf("        --index [index]           The starting index to add the entry or\n");
	printf("        -i [index]                entries to (shifts the other contents down\n");
	printf("                                  a position). If this is not specified or\n");
	printf("                                  is outside the entry list bounds, the\n");
	printf("                                  new entry will be appended to the end.\n");
	printf("\n");
	printf("        --list                    Signifies that the incoming source is a\n");
	printf("        -l                        list of files, rather than a single file\n");
	printf("                                  or stream.\n");
}

wadtool_t WADTOOL_Add = {
	"add",
	"Adds an entry or list of entries to a WAD.",
	&call,
	&usage,
	&help,
};
