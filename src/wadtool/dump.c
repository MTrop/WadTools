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
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "wadtool.h"
#include "common.h"
#include "wad/wad.h"
#include "wad/waderrno.h"
#include "wadio/wadstream.h"

extern int errno;
extern int waderrno;

#define ERRORDUMP_NONE        		0
#define ERRORDUMP_NO_FILENAME 		1
#define ERRORDUMP_BAD_SWITCH		2
#define ERRORDUMP_MISSING_PARAMETER	3
#define ERRORDUMP_STREAM_ERROR		4
#define ERRORDUMP_NOTHING_PRINTED	5
#define ERRORDUMP_WAD_ERROR   		10

#define SWITCH_INDEX				"-i"
#define SWITCH_INDEX2				"--index"
#define SWITCH_NAME					"-n"
#define SWITCH_NAME2				"--name"
#define SWITCH_STARTFROM			"-s"
#define SWITCH_STARTFROM2			"--start"
#define SWITCH_STARTFROMINDEX		"-si"
#define SWITCH_STARTFROMINDEX2		"--start-index"
#define SWITCH_STARTFROMNAME		"-sn"
#define SWITCH_STARTFROMNAME2		"--start-name"

typedef struct
{
	/** WAD filename. */
	char *filename;
	/** The WAD to use. */
	wad_t *wad;

	/** Entry criteria. */
	char *criteria;
	/** Entry type. */
	entry_search_type_t criteria_entrytype;
	/** Entry start criteria. */
	char *startfrom;
	/** Entry type. */
	entry_search_type_t startfrom_entrytype;

} wadtool_options_dump_t;

static void strupper(char* str)
{
	while (*str)
	{
		*str = toupper(*str);
		str++;
	}
}

static int exec(wadtool_options_dump_t *options)
{
	int start, index;
	if (!options->startfrom)
		start = 0;
	else
	{
		start = WADTools_FindEntryIndex(options->wad, options->startfrom_entrytype, options->startfrom, 0);
		if (start < 0)
			start = 0;
	}

	if (!options->criteria)
	{
		fprintf(stderr, "ERROR: Expected entry or index.\n");
		return ERRORDUMP_MISSING_PARAMETER;
	}
	else
	{
		index = WADTools_FindEntryIndex(options->wad, options->criteria_entrytype, options->criteria, start);
	}

	if (index < 0)
	{
		fprintf(stderr, "ERROR: Nothing to print.\n");
		return ERRORDUMP_NOTHING_PRINTED;
	}

	wadentry_t *entry = WAD_GetEntry(options->wad, index);
	if (!entry)
	{
		fprintf(stderr, "ERROR: Could not find entry.\n");
		return ERRORDUMP_MISSING_PARAMETER;
	}

	stream_t *stream = STREAM_OpenWADStream(options->wad, entry);
	if (!stream)
	{
		fprintf(stderr, "ERROR: Could not read from WAD.\n");
		return ERRORDUMP_STREAM_ERROR;
	}

#ifdef _WIN32
	_setmode(_fileno(stdout), _O_BINARY);
#endif

	int b;
	unsigned char buf[8192];
	while ((b = STREAM_Read(stream, buf, 1, 8192)) > 0)
	{
		if (fwrite(buf, 1, b, stdout) != b)
		{
			STREAM_Close(stream);
			fprintf(stderr, "ERROR: Could not write to STDOUT.\n");
			return ERRORDUMP_STREAM_ERROR;
		}
		fflush(stdout);
	}

#ifdef _WIN32
	_setmode(_fileno(stdout), _O_TEXT);
#endif

	return ERRORDUMP_NONE;
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
			if (matcharg(argparser, SWITCH_INDEX) || matcharg(argparser, SWITCH_INDEX2))
				options->criteria_entrytype = ET_INDEX;
			else if (matcharg(argparser, SWITCH_NAME) || matcharg(argparser, SWITCH_NAME2))
				options->criteria_entrytype = ET_NAME;
			else if (matcharg(argparser, SWITCH_STARTFROM) || matcharg(argparser, SWITCH_STARTFROM))
			{
				options->startfrom_entrytype = ET_DETECT;
				state = SWITCHSTATE_STARTFROM;
			}
			else if (matcharg(argparser, SWITCH_STARTFROMINDEX) || matcharg(argparser, SWITCH_STARTFROMINDEX2))
			{
				options->startfrom_entrytype = ET_INDEX;
				state = SWITCHSTATE_STARTFROM;
			}
			else if (matcharg(argparser, SWITCH_STARTFROMNAME) || matcharg(argparser, SWITCH_STARTFROMNAME2))
			{
				options->startfrom_entrytype = ET_NAME;
				state = SWITCHSTATE_STARTFROM;
			}
		}
		break;

		case SWITCHSTATE_STARTFROM:
		{
			options->startfrom = takearg(argparser);
			strupper(options->startfrom);
			state = SWITCHSTATE_INIT;
		}
		break;
	}

	if (state == SWITCHSTATE_STARTFROM)
	{
		fprintf(stderr, "ERROR: Expected entry or index.\n");
		return ERRORDUMP_MISSING_PARAMETER;
	}

	return 0;
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
			fprintf(stderr, "ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
		else
			fprintf(stderr, "ERROR: %s\n", strwaderror(waderrno));
		return ERRORDUMP_WAD_ERROR + waderrno;
	}
	nextarg(argparser);
	return 0;
}

static int call(arg_parser_t *argparser)
{
	wadtool_options_dump_t options = {NULL, NULL, NULL, ET_DETECT, NULL, ET_DETECT};

	int err;
	if ((err = parse_file(argparser, &options)))
	{
		return err;
	}

	if (currarg(argparser))
	{
		options.criteria = takearg(argparser);
		strupper(options.criteria);
	}
	else
	{
		WAD_Close(options.wad);
		fprintf(stderr, "ERROR: Expected entry or index.\n");
		return ERRORDUMP_MISSING_PARAMETER;
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
	printf("Usage: wad dump [wadfile] [entry] [switches]\n");
}

static void help()
{
	printf("[wadfile]:\n");
	printf("    The name of the WAD file to open.\n");
	printf("\n");
	printf("[entry]:\n");
	printf("    The entry to dump (name or index).\n");
	printf("\n");
	printf("[switches]:\n");
	printf("\n");
	printf("    Entry:\n");
	printf("\n");
	printf("        --index             Force [entry] to be interpreted as an index.\n");
	printf("        -i\n");
	printf("\n");
	printf("        --name              Force [entry] to be interpreted as a name.\n");
	printf("        -n\n");
	printf("\n");
	printf("    Search:\n");
	printf("\n");
	printf("        --start x           Starts the lookup from entry or index `x`.\n");
	printf("        -s x\n");
	printf("\n");
	printf("        --start-index x     Starts the lookup from index `x`.\n");
	printf("        -si x\n");
	printf("\n");
	printf("        --start-name x      Starts the lookup from the first entry called `x`.\n");
	printf("        -sn x\n");
}

wadtool_t WADTOOL_Dump = {
	"dump",
	"Dumps the contents of a WAD entry to STDOUT.",
	&call,
	&usage,
	&help,
};
