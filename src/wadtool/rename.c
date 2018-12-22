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
#include "rename.h"
#include "wad/wad.h"
#include "wad/waderrno.h"

extern int errno;
extern int waderrno;

#define ERRORRENAME_NONE        		0
#define ERRORRENAME_NO_FILENAME 		1
#define ERRORRENAME_MISSING_PARAMETER	2
#define ERRORRENAME_BAD_ENTRY			3
#define ERRORRENAME_WAD_ERROR   		10

#define SWITCH_INDEX				"-i"
#define SWITCH_INDEX2				"--index"
#define SWITCH_NAME					"-n"
#define SWITCH_NAME2				"--name"

typedef struct
{
	/** WAD filename. */
	char *filename;
	/** The WAD to use. */
	wad_t *wad;
	/** Entry/index to rename. */
	char *entry;
	/** Entry name search type. */
	entry_search_type_t entrytype;
	/** New name of entry. */
	char *newName;

} wadtool_options_rename_t;

static void strupper(char* str)
{
	while (*str)
	{
		*str = toupper(*str);
		str++;
	}
}

static int exec(wadtool_options_rename_t *options)
{
	wad_t *wad = options->wad;

	int sidx = WADTools_FindEntryIndex(wad, options->entrytype, options->entry, 0);
	if (sidx < 0)
	{
		fprintf(stderr, "ERROR: Could not find target entry.\n");
		return ERRORRENAME_BAD_ENTRY;
	}
	else if (sidx >= WAD_EntryCount(wad))
	{
		fprintf(stderr, "ERROR: Target entry index is out of range.\n");
		return ERRORRENAME_BAD_ENTRY;
	}

	char oldName[9];
	char newName[9];
	wadentry_t *srcEntry = WAD_GetEntry(wad, sidx);
	sprintf(oldName, "%-.8s", srcEntry->name);
	sprintf(newName, "%-.8s", options->newName);

	memcpy(srcEntry->name, newName, 8);

	if (WAD_CommitEntries(wad))
	{
		if (waderrno == WADERROR_FILE_ERROR)
			fprintf(stderr, "ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
		else
			fprintf(stderr, "ERROR: %s\n", strwaderror(waderrno));
		return ERRORRENAME_WAD_ERROR + waderrno;
	}
	printf("Renamed entry at index %d (%s) to %s in %s.\n", sidx, oldName, srcEntry->name, options->filename);

	return ERRORRENAME_NONE;
}

// If nonzero, bad parse.
static int parse_file(arg_parser_t *argparser, wadtool_options_rename_t *options)
{
	options->filename = currarg(argparser);
	if (!options->filename)
	{
		fprintf(stderr, "ERROR: No WAD file.\n");
		return ERRORRENAME_NO_FILENAME;
	}

	// Open a file.
	options->wad = WAD_Open(options->filename);

	if (!options->wad)
	{
		if (waderrno == WADERROR_FILE_ERROR)
			fprintf(stderr, "ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
		else
			fprintf(stderr, "ERROR: %s\n", strwaderror(waderrno));
		return ERRORRENAME_WAD_ERROR + waderrno;
	}
	nextarg(argparser);
	return 0;
}

// If nonzero, bad parse.
static int parse_switches(arg_parser_t *argparser, wadtool_options_rename_t *options)
{
	if (!currarg(argparser))
	{
		fprintf(stderr, "ERROR: Missing target entry.\n");
		return ERRORRENAME_MISSING_PARAMETER;
	}

	options->entry = takearg(argparser);
	strupper(options->entry);

	if (!currarg(argparser))
	{
		fprintf(stderr, "ERROR: Missing new name.\n");
		return ERRORRENAME_MISSING_PARAMETER;
	}

	options->newName = takearg(argparser);
	strupper(options->newName);

	while (currarg(argparser))
	{
		if (matcharg(argparser, SWITCH_INDEX) || matcharg(argparser, SWITCH_INDEX2))
			options->entrytype = ET_INDEX;
		else if (matcharg(argparser, SWITCH_NAME) || matcharg(argparser, SWITCH_NAME2))
			options->entrytype = ET_NAME;
	}

	return 0;
}

static int call(arg_parser_t *argparser)
{
	wadtool_options_rename_t options = {NULL, NULL, NULL, ET_DETECT, NULL};

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
	printf("Usage: wad rename [filename] [entry] [newname] [switches]\n");
}

static void help()
{
	printf("[filename]: \n");
	printf("    The name of the WAD file to rename entries in.\n");
	printf("\n");
	printf("[entry]: \n");
	printf("    The target entry index or name.\n");
	printf("\n");
	printf("[newname]: \n");
	printf("    The new name of the entry.\n");
	printf("\n");
	printf("[switches]: \n");
	printf("\n");
	printf("    Entry:\n");
	printf("\n");
	printf("        --index             Force [entry] to be interpreted as an index.\n");
	printf("        -i\n");
	printf("\n");
	printf("        --name              Force [entry] to be interpreted as a name.\n");
	printf("        -n\n");
}

wadtool_t WADTOOL_Rename = {
	"rename",
	"Renames an entry in a WAD file.",
	&call,
	&usage,
	&help,
};
