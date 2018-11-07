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
#include "list_common.h"
#include "wad/wad_config.h"
#include "wad/wad.h"
#include "wad/waderrno.h"

extern int errno;
extern int waderrno;

#define ERRORLIST_NONE          0
#define ERRORLIST_NO_FILENAME   1
#define ERRORLIST_BAD_SWITCH    2
#define ERRORLIST_BAD_SORT    	3
#define ERRORLIST_WAD_ERROR     10

#define SWITCH_RANGE			"--range"
#define SWITCH_RANGE2		    "-r"

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

	/** Range start. */
	int range_start;
	/** Range end. */
	int range_count;

	/** Reverse output. */
	int reverse;
	/** Sort function. */
	int (*sortfunc)(const void*, const void*);

} wadtool_options_list_t;

static int exec(wadtool_options_list_t *options)
{
	wad_t *wad = options->wad;
	int start, len;
	start = options->range_start;
	len = options->range_count;
	if (start < 0 || start + len > WAD_EntryCount(wad))
		len = WAD_EntryCount(wad) - start;
	
	if (!len)
	{
		printf("No entries.\n");
		return ERRORLIST_NONE;
	}

	int i;
	int count = 0;
	listentry_t *entrydata = (listentry_t*)WAD_MALLOC(sizeof(listentry_t) * len);
	for (i = start; i < start + len; i++)
	{
		entrydata[count].index = i;
		entrydata[count].entry = wad->entries[i];
		count++;
	}
	listentry_t **entries = listentry_shadow(entrydata, len);
	qsort(entries, len, sizeof(listentry_t*), options->sortfunc);

	if (!options->no_header && !options->inline_header)
		printf("Entries in %s, %d to %d\n", options->filename, start, start + len - 1);

	listentries_print(entries, count, count, options->listflags, options->no_header, options->inline_header, options->reverse);

	WAD_FREE(entries);
	WAD_FREE(entrydata);
	return ERRORLIST_NONE;
}

// If nonzero, bad parse.
static int parse_file(arg_parser_t *argparser, wadtool_options_list_t *options)
{
	options->filename = currarg(argparser);
	if (!options->filename)
	{
		fprintf(stderr, "ERROR: No WAD file.\n");
		return ERRORLIST_NO_FILENAME;
	}

	// Open a shallow mapping.
	options->wad = WAD_OpenMap(options->filename);

	if (!options->wad)
	{
		if (waderrno == WADERROR_FILE_ERROR)
			fprintf(stderr, "ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
		else
			fprintf(stderr, "ERROR: %s\n", strwaderror(waderrno));
		return ERRORLIST_WAD_ERROR + waderrno;
	}
	nextarg(argparser);
	return 0;
}

#define SWITCHSTATE_INIT		0
#define SWITCHSTATE_RANGE		1
#define SWITCHSTATE_RANGE2		2
#define SWITCHSTATE_SORTTYPE	3

// If nonzero, bad parse.
static int parse_switches(arg_parser_t *argparser, wadtool_options_list_t *options)
{
	int state = SWITCHSTATE_INIT;
	while (argparser->arg) switch (state)
	{
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
			{
				state = SWITCHSTATE_INIT;
			}
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
			{
				state = SWITCHSTATE_INIT;
			}
			else
			{
				int i = atoi(currarg(argparser));
				nextarg(argparser);
				options->range_count = i;
				state = SWITCHSTATE_INIT;
			}
		}
		break;

		case SWITCHSTATE_SORTTYPE:
		{
			if (matcharg(argparser, SORT_INDEX))
			{
				options->sortfunc = &listentry_sort_index;
				state = SWITCHSTATE_INIT;
			}
			else if (matcharg(argparser, SORT_NAME))
			{
				options->sortfunc = &listentry_sort_name;
				state = SWITCHSTATE_INIT;
			}
			else if (matcharg(argparser, SORT_LENGTH))
			{
				options->sortfunc = &listentry_sort_length;
				state = SWITCHSTATE_INIT;
			}
			else if (matcharg(argparser, SORT_OFFSET))
			{
				options->sortfunc = &listentry_sort_offset;
				state = SWITCHSTATE_INIT;
			}
			else
			{
				printf("ERROR: Bad sort type: %s\n", currarg(argparser));
				return ERRORLIST_BAD_SORT;
			}
		}
		break;
	}

	if (state == SWITCHSTATE_SORTTYPE)
	{
		printf("ERROR: Expected type after sort switch.\n");
		return ERRORLIST_BAD_SWITCH;
	}

	if (state == SWITCHSTATE_RANGE)
	{
		printf("ERROR: Expected arguments after range switch.\n");
		return ERRORLIST_BAD_SWITCH;
	}

	return 0;
}

static int call(arg_parser_t *argparser)
{
	wadtool_options_list_t options = {NULL, NULL, 0, 0, 0, 0, 0, 0, &listentry_sort_index};

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
	printf("        ..............................\n");
	printf("\n");
	printf("        --no-header         Do not print the output header (and footer).\n");
	printf("        -nh\n");
	printf("\n");
	printf("        --inline-header     Print the header names on the output line.\n");
	printf("        -ih\n");
	printf("\n");
	printf("    If none of the previous options are specified, `--all` is implied.\n");
	printf("\n");
	printf("    Filters:\n");
	printf("\n");
	printf("        --range x c         Only selects from the entries from index x,\n");
	printf("        -r x c              c entries (for example, `--range 0 20` means index\n");
	printf("                            0 up to 19, 20 entries). If `c` is not specified,\n");
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
