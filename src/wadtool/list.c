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

#define ERRORLIST_NONE          0
#define ERRORLIST_NO_FILENAME   1
#define ERRORLIST_BAD_SWITCH    2
#define ERRORLIST_WAD_ERROR     10

#define LISTFLAG_INDICES    (1 << 0)
#define LISTFLAG_NAMES      (1 << 1)
#define LISTFLAG_LENGTHS    (1 << 2)
#define LISTFLAG_OFFSETS    (1 << 3)
#define LISTFLAG_ALL        ( LISTFLAG_INDICES | LISTFLAG_NAMES | LISTFLAG_LENGTHS | LISTFLAG_OFFSETS )

#define COMPARE_INT(x,y)	((x) == (y) ? 0 : ((x) < (y) ? -1 : 1))

typedef struct 
{
	/** WAD filename. */
	char *filename;
	/** The WAD to list. */
	wad_t *wad;
	/** List info flags. */
	int listflags;

	/** If nonzero, don't print header. */
	int no_header;
	/** If nonzero, print inline header. */
	int inline_header;
	/** If nonzero, detect map enries only. */
	int maps_only;
	/** Range start. */
	int range_start;
	/** Range end. */
	int range_end;
	/** Sort function. */
	int (*sortfunc)(void*, void*);

} wadtool_options_list_t;

// Sort wadentries by index (do nothing).
static int wadentry_sort_index(void *a, void *b)
{
	// Change nothing.
	return 0;
}

// Sort wadentries by name.
static int wadentry_sort_name(void *a, void *b)
{
	wadentry_t *x = *(wadentry_t**)a;
	wadentry_t *y = *(wadentry_t**)b;
	return strcmp(x->name, y->name);
}

// Sort wadentries by length.
static int wadentry_sort_length(void *a, void *b)
{
	wadentry_t *x = *(wadentry_t**)a;
	wadentry_t *y = *(wadentry_t**)b;
	return COMPARE_INT(x->length, y->length);
}

// Sort wadentries by starting offset.
static int wadentry_sort_offset(void *a, void *b)
{
	wadentry_t *x = *(wadentry_t**)a;
	wadentry_t *y = *(wadentry_t**)b;
	return COMPARE_INT(x->offset, y->offset);
}

static int exec(wadtool_options_list_t *options)
{
	printf("ERROR: NOT SUPPORTED YET!\n");
	return -1;
}

#define SWITCHSTATE_INIT		0
#define SWITCHSTATE_RANGE		1
#define SWITCHSTATE_SORTTYPE	2

#define SWITCH_PREFIX			"-"

#define SWITCH_INDICES			"--indices"
#define SWITCH_INDICES2			"-i"
#define SWITCH_NAMES			"--names"
#define SWITCH_NAMES2			"-n"
#define SWITCH_LENGTHS			"--lengths"
#define SWITCH_LENGTHS2			"-l"
#define SWITCH_OFFSETS			"--offsets"
#define SWITCH_OFFSETS2			"-o"

// If nonzero, bad parse.
static int switches(arg_parser_t *argparser, wadtool_options_list_t *options)
{
	int state = SWITCHSTATE_INIT;
	while (argparser->arg) switch (state)
	{
		// TODO: Finish these.
		case SWITCHSTATE_INIT:
		{
			//if (matcharg(argparser, SWITCH_INDICES) || matcharg(argparser, SWITCH_INDICES2))

		}
		break;
		
		case SWITCHSTATE_RANGE:
		{

		}
		break;
		
		case SWITCHSTATE_SORTTYPE:
		{

		}
		break;
	}

	if (state != SWITCHSTATE_INIT)
	{
		printf("ERROR: Expected arguments\n");
	}

	return 0;
}

static int call(arg_parser_t *argparser)
{
	wadtool_options_list_t options = {NULL, NULL, 0, 0, 0, 0, 0, 0, &wadentry_sort_index};

	options.filename = currarg(argparser);
	if (!options.filename)
	{
		fprintf(stderr, "ERROR: No WAD file.\n");
		return ERRORLIST_NO_FILENAME;
	}

	// Open a shallow mapping.
	options.wad = WAD_OpenMap(options.filename);
	
	if (!options.wad)
	{
		if (waderrno == WADERROR_FILE_ERROR)
			printf("ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
		else
			printf("ERROR: %s\n", strwaderror(waderrno));
		return ERRORLIST_WAD_ERROR + waderrno;
	}

	int err;
	nextarg(argparser);
	if (err = switches(argparser, &options)) // the equals is intentional.
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
	printf("Usage: wad list [filename] [switches]\n");
}

static void help()
{
	printf("[filename]: \n");
	printf("    The name of the WAD file to list the entries of.\n");
	printf("\n");
	printf("[switches]: \n");
	printf("\n");
	printf("    Print options:\n");
	printf("\n");
	printf("    --indices           Prints the entry indexes.\n");
	printf("    -i\n");
	printf("\n");
	printf("    --names             Prints the entry names.\n");
	printf("    -n\n");
	printf("\n");
	printf("    --length            Prints the entry content lengths.\n");
	printf("    -l\n");
	printf("\n");
	printf("    --offsets           Prints the entry content offsets.\n");
	printf("    -o\n");
	printf("\n");
	printf("    --all               Prints everything (default).\n");
	printf("\n");
	printf("    If none of the previous options are specified, `--all` is implied.\n");
	printf("\n");
	printf("    Filters:\n");
	printf("\n");
	printf("    --no-header         Do not print the output header.\n");
	printf("    -nh\n");
	printf("\n");
	printf("    --inline-header     Print the header names on the output line.\n");
	printf("    -ih\n");
	printf("\n");
	printf("    --maps              Only prints the entries that are considered map header\n");
	printf("    -m                  entries (guessed).\n");
	printf("\n");
	printf("    --range=x,y         Only prints the entries from index x to y,\n");
	printf("    -r x y              inclusive-exclusive (for example, `--range=0,20` means\n");
	printf("                        index 0 up to 19). If `y` is not specified, assumes\n");
	printf("                        end of entry list.\n");
	printf("\n");
	printf("    Other:\n");
	printf("\n");
	printf("    --sort=type         Sorts output by entry values.\n");
	printf("    -s type             Valid types:\n");
	printf("\n");
	printf("                        index   - (default) sort by index order (no sort).\n");
	printf("                        name    - sort by entry name.\n");
	printf("                        length  - sort by entry content length.\n");
	printf("                        offset  - sort by entry content offset.\n");
	printf("\n");
	printf("    --reverse-sort      Reverses sort order.\n");
	printf("    -r\n");
}

wadtool_t WADTOOL_List = {
	"list",
	"Lists the entries in a WAD.",
	&call,
	&usage,
	&help,
};
