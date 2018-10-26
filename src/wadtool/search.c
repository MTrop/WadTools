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

#define ERRORSEARCH_NONE			0
#define ERRORSEARCH_NO_FILENAME		1
#define ERRORSEARCH_BAD_SWITCH		2
#define ERRORSEARCH_BAD_SORT		3
#define ERRORSEARCH_WAD_ERROR		10

#define MODE_MAPS			    	"maps"
#define MAPENTRY_SEARCHNAME			"THINGS"
#define MAPENTRY_SEARCHNAME2		"TEXTMAP"

/**
 * Search types.
 */
typedef enum 
{
	ST_MAPS,

} searchtype_t;

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

	/** Reverse output. */
	int reverse;
	/** Sort function. */
	int (*sortfunc)(const void*, const void*);

	/** Search type. */
	searchtype_t searchtype;

} wadtool_options_search_t;


static int exec(wadtool_options_search_t *options)
{
	wad_t *wad = options->wad;

	int i, len = WAD_EntryCount(wad);
	listentry_t **entries;
	listentry_t *entrydata;
	int count = 0;

	switch (options->searchtype)
	{
		case ST_MAPS:
		{
			for (i = 1; i < len; i++)
			{
				if (strncmp(wad->entries[i]->name, MAPENTRY_SEARCHNAME, 8) == 0)
					count++;
				else if (strncmp(wad->entries[i]->name, MAPENTRY_SEARCHNAME2, 8) == 0)
					count++;
			}

			if (!count)
			{
				printf("No entries.\n");
				return ERRORSEARCH_NONE;
			}

			entrydata = (listentry_t*)WAD_MALLOC(sizeof(listentry_t) * count);
			count = 0;
			for (i = 1; i < len; i++)
			{
				if (strncmp(wad->entries[i]->name, MAPENTRY_SEARCHNAME, 8) == 0)
				{
					entrydata[count].index = i - 1;
					entrydata[count].entry = wad->entries[i - 1];
					count++;
				}
				else if (strncmp(wad->entries[i]->name, MAPENTRY_SEARCHNAME2, 8) == 0)
				{
					entrydata[count].index = i - 1;
					entrydata[count].entry = wad->entries[i - 1];
					count++;
				}
			}
			entries = listentry_shadow(entrydata, count);
			qsort(entries, count, sizeof(listentry_t*), options->sortfunc);
		}
	}

	if (!options->no_header && !options->inline_header)
	{
		printf("Entries in %s\n", options->filename);
		switch (options->searchtype)
		{
			case ST_MAPS: printf("Listing MAPS.\n"); break;
		}
	}

	listentries_print(entries, count, options->listflags, options->no_header, options->inline_header, options->reverse);

	WAD_FREE(entries);
	WAD_FREE(entrydata);
	return ERRORSEARCH_NONE;
}

#define SWITCHSTATE_INIT		0
#define SWITCHSTATE_SORTTYPE	1

// If nonzero, bad parse.
static int switches(arg_parser_t *argparser, wadtool_options_search_t *options)
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
			else if (matcharg(argparser, SWITCH_SORT) || matcharg(argparser, SWITCH_SORT2))
				state = SWITCHSTATE_SORTTYPE;
			else
			{
				printf("ERROR: Bad switch: %s\n", currarg(argparser));
				return ERRORSEARCH_BAD_SWITCH;
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
				return ERRORSEARCH_BAD_SORT;
			}
		}
		break;
	}

	if (state == SWITCHSTATE_SORTTYPE)
	{
		printf("ERROR: Expected type after sort switch.\n");
		return ERRORSEARCH_BAD_SWITCH;
	}

	return 0;
}

static int call(arg_parser_t *argparser)
{
	wadtool_options_search_t options = {};

	options.filename = currarg(argparser);
	if (!options.filename)
	{
		fprintf(stderr, "ERROR: No WAD file.\n");
		return ERRORSEARCH_NO_FILENAME;
	}

	// Open a shallow mapping.
	options.wad = WAD_OpenMap(options.filename);

	if (!options.wad)
	{
		if (waderrno == WADERROR_FILE_ERROR)
			printf("ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
		else
			printf("ERROR: %s\n", strwaderror(waderrno));
		return ERRORSEARCH_WAD_ERROR + waderrno;
	}

	int err;
	nextarg(argparser);
	if (err = switches(argparser, &options)) // the single equals is intentional.
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
	printf("Usage: wad search [mode] [filename] [switches]\n");
}

static void help()
{
	// TODO: Finish this.
	printf("[mode]: \n");
	printf("    *****FINISH ME*********.\n");
	printf("\n");
	printf("[filename]: \n");
	printf("    The name of the WAD file to search the entries of.\n");
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

wadtool_t WADTOOL_Search = {
	"search",
	"Search a WAD for entries that fit certain criteria.",
	&call,
	&usage,
	&help,
};
