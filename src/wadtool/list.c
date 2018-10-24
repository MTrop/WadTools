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
#include "wad/wadconfig.h"
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

#define SWITCHSTATE_INIT		0
#define SWITCHSTATE_RANGE		1
#define SWITCHSTATE_RANGE2		2
#define SWITCHSTATE_SORTTYPE	3

#define SWITCH_PREFIX			"-"

#define SWITCH_INDICES			"--indices"
#define SWITCH_INDICES2			"-i"
#define SWITCH_NAMES			"--names"
#define SWITCH_NAMES2			"-n"
#define SWITCH_LENGTHS			"--lengths"
#define SWITCH_LENGTHS2			"-l"
#define SWITCH_OFFSETS			"--offsets"
#define SWITCH_OFFSETS2			"-o"
#define SWITCH_ALL			    "--all"

#define SWITCH_NOHEADER		    "--no-header"
#define SWITCH_NOHEADER2	    "-nh"
#define SWITCH_INLINEHEADER	    "--inline-header"
#define SWITCH_INLINEHEADER2    "-ih"
#define SWITCH_MAPS			    "--maps"
#define SWITCH_MAPS2		    "-m"
#define SWITCH_REVERSESORT		"--reverse-sort"
#define SWITCH_REVERSESORT2	    "-rs"
#define SWITCH_RANGE			"--range"
#define SWITCH_RANGE2		    "-r"
#define SWITCH_SORT				"--sort"
#define SWITCH_SORT2		    "-s"

#define SORT_INDEX				"index"
#define SORT_NAME				"name"
#define SORT_LENGTH				"length"
#define SORT_OFFSET				"offset"

#define MAPENTRY_SEARCHNAME		"THINGS"
#define MAPENTRY_SEARCHNAME2	"TEXTMAP"

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
	/** Reverse output. */
	int reverse;
	/** Sort function. */
	int (*sortfunc)(const void*, const void*);

} wadtool_options_list_t;

typedef struct 
{	
	int index;
	wadentry_t *entry;

} listentry_t;

// Sort entries by index.
static int entry_sort_index(const void *a, const void *b)
{
	listentry_t *x = *(listentry_t**)a;
	listentry_t *y = *(listentry_t**)b;
	return COMPARE_INT(x->index, y->index);
}

// Sort wadentries by name.
static int entry_sort_name(const void *a, const void *b)
{
	listentry_t *x = *(listentry_t**)a;
	listentry_t *y = *(listentry_t**)b;
	return strncmp(x->entry->name, y->entry->name, 8);
}

// Sort wadentries by length.
static int entry_sort_length(const void *a, const void *b)
{
	listentry_t *x = *(listentry_t**)a;
	listentry_t *y = *(listentry_t**)b;
	return COMPARE_INT(x->entry->length, y->entry->length);
}

// Sort wadentries by starting offset.
static int entry_sort_offset(const void *a, const void *b)
{
	listentry_t *x = *(listentry_t**)a;
	listentry_t *y = *(listentry_t**)b;
	return COMPARE_INT(x->entry->offset, y->entry->offset);
}

static void print_entry(wadtool_options_list_t *options, listentry_t *listentry, int i)
{
	if (!options->listflags || (options->listflags & LISTFLAG_INDICES))
	{
		if (!options->no_header && options->inline_header)
			printf("Index ");
		printf("%-10d ", listentry->index);
	}
	if (!options->listflags || (options->listflags & LISTFLAG_NAMES))
	{
		if (!options->no_header && options->inline_header)
			printf("Name ");
		printf("%-8.8s ", listentry->entry->name);
	}
	if (!options->listflags || (options->listflags & LISTFLAG_LENGTHS))
	{
		if (!options->no_header && options->inline_header)
			printf("Length ");
		printf("%-10d ", listentry->entry->length);
	}
	if (!options->listflags || (options->listflags & LISTFLAG_OFFSETS))
	{
		if (!options->no_header && options->inline_header)
			printf("Offset ");
		printf("%-10d ", listentry->entry->offset);
	}
	printf("\n");
}

static int exec(wadtool_options_list_t *options)
{
	wad_t *wad = options->wad;
	int start, end, len;
	start = options->range_start;
	if (options->range_end <= 0)
		end = wad->header.entry_count;
	else
		end = options->range_end;
	
	if (end <= start)
		return 0;

	len = end - start;
	int i;

	listentry_t **entries;
	int count = 0;
	if (options->maps_only)
	{
		// TODO: List and sort.
	}
	else
	{
		// TODO: List and sort.
	}

	if (!options->no_header && !options->inline_header)
	{
		printf("Index      Name     Size       Offset\n");
		printf("-----------------------------------------\n");
	}

	if (options->reverse) for (i = count - 1; i >= 0; i--)
		print_entry(options, entries[i], i);
	else for (i = 0; i < count; i++)
		print_entry(options, entries[i], i);

	WAD_FREE(entries);
	return ERRORLIST_NONE;
}

// If nonzero, bad parse.
static int switches(arg_parser_t *argparser, wadtool_options_list_t *options)
{
	int state = SWITCHSTATE_INIT;
	while (argparser->arg) switch (state)
	{
		// TODO: Finish these.
		case SWITCHSTATE_INIT:
		{
			if (matcharg(argparser, SWITCH_INDICES) || matcharg(argparser, SWITCH_INDICES2))
				options->listflags |= LISTFLAG_INDICES;
			else if (matcharg(argparser, SWITCH_NAMES) || matcharg(argparser, SWITCH_NAMES2))
				options->listflags |= LISTFLAG_NAMES;
			else if (matcharg(argparser, SWITCH_LENGTHS) || matcharg(argparser, SWITCH_LENGTHS2))
				options->listflags |= LISTFLAG_LENGTHS;
			else if (matcharg(argparser, SWITCH_OFFSETS) || matcharg(argparser, SWITCH_OFFSETS2))
				options->listflags |= LISTFLAG_OFFSETS;
			else if (matcharg(argparser, SWITCH_ALL))
				options->listflags |= LISTFLAG_ALL;
			else if (matcharg(argparser, SWITCH_NOHEADER) || matcharg(argparser, SWITCH_NOHEADER2))
				options->no_header = 1;
			else if (matcharg(argparser, SWITCH_INLINEHEADER) || matcharg(argparser, SWITCH_INLINEHEADER2))
				options->inline_header = 1;
			else if (matcharg(argparser, SWITCH_MAPS) || matcharg(argparser, SWITCH_MAPS2))
				options->maps_only = 1;
			else if (matcharg(argparser, SWITCH_REVERSESORT) || matcharg(argparser, SWITCH_REVERSESORT2))
				options->reverse = 1;
			else if (matcharg(argparser, SWITCH_RANGE) || matcharg(argparser, SWITCH_RANGE2))
				state = SWITCHSTATE_RANGE;
			else if (matcharg(argparser, SWITCH_SORT) || matcharg(argparser, SWITCH_SORT2))
				state = SWITCHSTATE_SORTTYPE;
			else
			{
				printf("ERROR: Bad switch: %s\n", currarg(argparser));
				return ERRORLIST_BAD_SWITCH;
			}
		}
		break;
		
		case SWITCHSTATE_RANGE:
		{
			if (currargstart(argparser, SWITCH_PREFIX))
				state = SWITCHSTATE_INIT;
			else
			{
				int i = atoi(currarg(argparser));
				nextarg(argparser);
				options->range_start = i;
				state = SWITCHSTATE_RANGE2;
			}
		}
		break;
		
		case SWITCHSTATE_RANGE2:
		{
			if (currargstart(argparser, SWITCH_PREFIX))
				state = SWITCHSTATE_INIT;
			else
			{
				int i = atoi(currarg(argparser));
				nextarg(argparser);
				options->range_end = i;
				state = SWITCHSTATE_INIT;
			}
		}
		break;

		case SWITCHSTATE_SORTTYPE:
		{
			if (matcharg(argparser, SORT_INDEX))
			{
				options->sortfunc = &entry_sort_index;
				state = SWITCHSTATE_INIT;
			}
			else if (matcharg(argparser, SORT_NAME))
			{
				options->sortfunc = &entry_sort_name;
				state = SWITCHSTATE_INIT;
			}
			else if (matcharg(argparser, SORT_LENGTH))
			{
				options->sortfunc = &entry_sort_length;
				state = SWITCHSTATE_INIT;
			}
			else if (matcharg(argparser, SORT_OFFSET))
			{
				options->sortfunc = &entry_sort_offset;
				state = SWITCHSTATE_INIT;
			}
			else
			{
				printf("ERROR: Bad sort type: %s\n", currarg(argparser));
				return ERRORLIST_BAD_SWITCH;
			}
		}
		break;
	}

	if (state != SWITCHSTATE_INIT)
	{
		printf("ERROR: Expected arguments after switch.\n");
		return ERRORLIST_BAD_SWITCH;
	}

	return 0;
}

static int call(arg_parser_t *argparser)
{
	wadtool_options_list_t options = {NULL, NULL, 0, 0, 0, 0, 0, 0, 0, &entry_sort_index};

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
	printf("        --indices           Prints the entry indexes.\n");
	printf("        -i\n");
	printf("\n");
	printf("        --names             Prints the entry names.\n");
	printf("        -n\n");
	printf("\n");
	printf("        --length            Prints the entry content lengths.\n");
	printf("        -l\n");
	printf("\n");
	printf("        --offsets           Prints the entry content offsets.\n");
	printf("        -o\n");
	printf("\n");
	printf("        --all               Prints everything (default).\n");
	printf("\n");
	printf("    If none of the previous options are specified, `--all` is implied.\n");
	printf("\n");
	printf("    Filters:\n");
	printf("\n");
	printf("        --no-header         Do not print the output header.\n");
	printf("        -nh\n");
	printf("\n");
	printf("        --inline-header     Print the header names on the output line.\n");
	printf("        -ih\n");
	printf("\n");
	printf("        --maps              Only prints the entries that are considered map\n");
	printf("        -m                  header entries (guessed).\n");
	printf("\n");
	printf("        --range=x,y         Only prints the entries from index x to y,\n");
	printf("        -r x y              inclusive-exclusive (for example, `--range=0,20`\n");
	printf("                            means index 0 up to 19). If `y` is not specified,\n");
	printf("                            assumes end of entry list.\n");
	printf("\n");
	printf("    Other:\n");
	printf("\n");
	printf("        --sort [type]       Sorts output by entry values.\n");
	printf("        -s [type]           Valid values for [type]:\n");
	printf("\n");
	printf("                            index   - (default) sort by index order (no sort).\n");
	printf("                            name    - sort by entry name.\n");
	printf("                            length  - sort by entry content length.\n");
	printf("                            offset  - sort by entry content offset.\n");
	printf("\n");
	printf("        --reverse-sort      Reverses sort order.\n");
	printf("        -rs\n");
}

wadtool_t WADTOOL_List = {
	"list",
	"Lists the entries in a WAD.",
	&call,
	&usage,
	&help,
};
