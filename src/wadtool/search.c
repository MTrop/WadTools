/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include "wadtool.h"
#include "list_common.h"
#include "wad/wad_config.h"
#include "wad/wad.h"
#include "wad/waderrno.h"

extern int errno;
extern int waderrno;

#define ERRORSEARCH_NONE				0
#define ERRORSEARCH_NO_FILENAME			1
#define ERRORSEARCH_BAD_SWITCH			2
#define ERRORSEARCH_BAD_MODE			3
#define ERRORSEARCH_MISSING_PARAMETER	4
#define ERRORSEARCH_BAD_SORT			5
#define ERRORSEARCH_WAD_ERROR			10

#define ERRORSEARCH_MAP_NOT_FOUND		30


#define MODE_MAP			    		"map"
#define MODE_MAPS			    		"maps"
#define MODE_NAME			    		"name"
#define MODE_NAMESPACE					"namespace"

#define MAPENTRY_SEARCHNAME				"THINGS"
#define MAPENTRY_SEARCHNAME2			"TEXTMAP"

// MUST BE ALPHABETICAL!
#define MAP_ENTRY_NAMES_COUNT 22
static char* map_entry_names[MAP_ENTRY_NAMES_COUNT] = 
{
	"BEHAVIOR",
	"BLOCKMAP",
	"DIALOGUE",
	"ENDMAP",
	"GL_NODES",
	"GL_PVS",
	"GL_SEGS",
	"GL_SSECT",
	"GL_VERT",
	"LINEDEFS",
	"NODES",
	"PWADINFO",
	"REJECT",	
	"SCRIPTS",
	"SECTORS",
	"SEGS",
	"SIDEDEFS",
	"SSECTORS",
	"TEXTMAP",
	"THINGS",
	"VERTEXES",
	"ZNODES",
};

// Find a map entry by name, add to list if found.
static int accum_map_entry(int* list, int index, wad_t *wad, char* name, int offset)
{
	int out = WAD_GetEntryIndexOffset(wad, name, offset);
	if (out >= 0)
	{
		list[index] = out;
		return out;
	}
	return -1;
}

/**
 * Search types.
 */
typedef enum 
{
	ST_NONE,
	ST_MAPS,
	ST_MAP,
	ST_NAME,
	ST_NAMESPACE,

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
	/** Search criteria. */
	char *criterion0;
	char *criterion1;
	/** Print limit. */
	size_t limit;

} wadtool_options_search_t;

static void strupper(char* str)
{
	while (*str)
	{
		*str = toupper(*str);
		str++;
	}
}

static int exec(wadtool_options_search_t *options)
{
	wad_t *wad = options->wad;

	int i, len = WAD_EntryCount(wad);
	listentry_t **entries;
	listentry_t *entrydata;
	int count;

	switch (options->searchtype)
	{
		/***** Nothing *****/
		default:
		case ST_NONE:
		{
			printf("NOT IMPLEMENTED!!!!!\n");
			return 666;
		}

		/***** Maps Search Mode *****/
		case ST_MAPS:
		{
			count = 0;
			for (i = 1; i < len; i++)
			{
				wadentry_t *entry = WAD_GetEntry(wad, i);
				if (strncmp(entry->name, MAPENTRY_SEARCHNAME, 8) == 0)
					count++;
				else if (strncmp(entry->name, MAPENTRY_SEARCHNAME2, 8) == 0)
					count++;
			}

			if (!count)
			{
				if (!options->no_header)
					printf("No entries.\n");
				return ERRORSEARCH_NONE;
			}

			entrydata = (listentry_t*)WAD_MALLOC(sizeof(listentry_t) * count);

			count = 0;
			for (i = 1; i < len; i++)
			{
				wadentry_t *entry = WAD_GetEntry(wad, i);
				if (strncmp(entry->name, MAPENTRY_SEARCHNAME, 8) == 0 || strncmp(entry->name, MAPENTRY_SEARCHNAME2, 8) == 0)
				{
					entrydata[count].index = i - 1;
					entrydata[count].entry = WAD_GetEntry(wad, i - 1);
					count++;
				}
			}
			entries = listentry_shadow(entrydata, count);
			qsort(entries, count, sizeof(listentry_t*), options->sortfunc);
		}
		break;

		case ST_MAP:
		{
			int offset = WAD_GetEntryIndex(wad, options->criterion0);
			if (offset < 0)
			{
				printf("ERROR: Map name %s not found!\n");
				return ERRORSEARCH_MAP_NOT_FOUND;
			}

			// Output array of indices, filled with -1 (same when int-sized).
			int indices[MAP_ENTRY_NAMES_COUNT];
			memset(indices, 0xFF, sizeof(int) * (MAP_ENTRY_NAMES_COUNT));
			count = 0;
			for (i = 0; i < MAP_ENTRY_NAMES_COUNT; i++)
			{
				int index = accum_map_entry(indices, count, wad, map_entry_names[i], offset);
				if (index >= offset)
					indices[count++] = index;
			}
			
			if (!count)
			{
				if (!options->no_header)
					printf("No entries.\n");
				return ERRORSEARCH_NONE;
			}

			wadentry_t *entry;
			// add one to count to accommodate header entry
			entrydata = (listentry_t*)WAD_MALLOC(sizeof(listentry_t) * (count + 1));
			for (i = 0; i < count; i++)
			{
				int index = indices[i];
				entry = WAD_GetEntry(wad, index);
				entrydata[i].index = index;
				entrydata[i].entry = entry;
			}
			entrydata[count].index = offset;
			entrydata[count].entry = WAD_GetEntry(wad, offset);
			count++;
			entries = listentry_shadow(entrydata, count);
			qsort(entries, count, sizeof(listentry_t*), options->sortfunc);
		}
		break;

		case ST_NAME:
		{
			char ename[9];
			ename[8] = '\0';

			count = 0;
			for (i = 0; i < len; i++)
			{
				wadentry_t *entry = WAD_GetEntry(wad, i);
				memcpy(ename, entry->name, 8);
				if (strstr(ename, options->criterion0) == ename) // starts with
					count++;
			}

			if (!count)
			{
				if (!options->no_header)
					printf("No entries.\n");
				return ERRORSEARCH_NONE;
			}

			entrydata = (listentry_t*)WAD_MALLOC(sizeof(listentry_t) * count);

			count = 0;
			for (i = 0; i < len; i++)
			{
				wadentry_t *entry = WAD_GetEntry(wad, i);
				memcpy(ename, entry->name, 8);
				if (strstr(ename, options->criterion0) == ename) // starts with
				{
					entrydata[count].index = i;
					entrydata[count].entry = entry;
					count++;
				}
			}
			entries = listentry_shadow(entrydata, count);
			qsort(entries, count, sizeof(listentry_t*), options->sortfunc);
		}
		break;

		case ST_NAMESPACE:
		{
			char sname[9], ename[9];
			sprintf(sname, "%.2s_START", options->criterion0);
			sprintf(ename, "%.2s_END", options->criterion0);

			int start_index = WAD_GetEntryIndex(wad, sname);
			int end_index = WAD_GetEntryIndex(wad, ename);

			count = end_index - start_index - 1;
			if (start_index < 0 || end_index < 0 || count < 0)
			{
				if (!options->no_header)
					printf("No entries.\n");
				return ERRORSEARCH_NONE;
			}

			entrydata = (listentry_t*)WAD_MALLOC(sizeof(listentry_t) * count);
			count = 0;
			for (i = start_index + 1; i < end_index; i++)
			{
				wadentry_t *entry = WAD_GetEntry(wad, i);
				entrydata[count].index = i;
				entrydata[count].entry = entry;
				count++;
			}
			entries = listentry_shadow(entrydata, count);
			qsort(entries, count, sizeof(listentry_t*), options->sortfunc);
		}
		break;

	}

	if (!options->no_header && !options->inline_header)
	{
		printf("Entries in %s\n", options->filename);
		switch (options->searchtype)
		{
			default:
			case ST_NONE: // Should not be seen.
				printf("[NO SEARCH TYPE]\n"); 
				break;
			case ST_MAPS: 
				printf("Listing MAPS.\n"); 
				break;
			case ST_MAP: 
				printf("Listing entries in map %s.\n", options->criterion0); 
				break;
			case ST_NAME: 
				printf("Listing entries starting with `%s`.\n", options->criterion0); 
				break;
			case ST_NAMESPACE:
				printf("Listing entries in namespace %.2s_START / %.2s_END.\n", options->criterion0, options->criterion0);
				break;
		}
	}

	// sanitize limit
	options->limit = options->limit <= 0 ? count : options->limit;

	listentries_print(entries, count, options->limit, options->listflags, options->no_header, options->inline_header, options->reverse);

	if (entries) WAD_FREE(entries);
	if (entrydata) WAD_FREE(entrydata);
	return ERRORSEARCH_NONE;
}

// If nonzero, bad parse.
static int parse_mode(arg_parser_t *argparser, wadtool_options_search_t *options)
{
	if (matcharg(argparser, MODE_MAPS))
	{
		options->searchtype = ST_MAPS;
		return 0;
	}
	else if (matcharg(argparser, MODE_MAP))
	{
		options->searchtype = ST_MAP;
		return 0;
	}
	else if (matcharg(argparser, MODE_NAME))
	{
		options->searchtype = ST_NAME;
		return 0;
	}
	else if (matcharg(argparser, MODE_NAMESPACE))
	{
		options->searchtype = ST_NAMESPACE;
		return 0;
	}
	else if (!currarg(argparser))
	{
		printf("ERROR: Expected mode.\n");
		return ERRORSEARCH_MISSING_PARAMETER;
	}
	else
	{
		printf("ERROR: Bad mode: %s\n", currarg(argparser));
		return ERRORSEARCH_BAD_MODE;
	}
}

// If nonzero, bad parse.
static int parse_file(arg_parser_t *argparser, wadtool_options_search_t *options)
{
	options->filename = currarg(argparser);
	if (!options->filename)
	{
		fprintf(stderr, "ERROR: No WAD file.\n");
		return ERRORSEARCH_NO_FILENAME;
	}

	// Open a shallow mapping.
	options->wad = WAD_OpenMap(options->filename);

	if (!options->wad)
	{
		if (waderrno == WADERROR_FILE_ERROR)
			printf("ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
		else
			printf("ERROR: %s\n", strwaderror(waderrno));
		return ERRORSEARCH_WAD_ERROR + waderrno;
	}
	nextarg(argparser);
	return 0;
}

#define SWITCHSTATE_INIT		0
#define SWITCHSTATE_SORTTYPE	1
#define SWITCHSTATE_LIMIT		2

// If nonzero, bad parse.
static int parse_switches(arg_parser_t *argparser, wadtool_options_search_t *options)
{
	int state = SWITCHSTATE_INIT;
	while (currarg(argparser)) switch (state)
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
			else if (matcharg(argparser, SWITCH_LIMIT))
				state = SWITCHSTATE_LIMIT;
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

		case SWITCHSTATE_LIMIT:
		{
			options->limit = atoi(takearg(argparser));
			state = SWITCHSTATE_INIT;
		}
		break;
	}

	if (state == SWITCHSTATE_SORTTYPE)
	{
		printf("ERROR: Expected type after `sort` switch.\n");
		return ERRORSEARCH_MISSING_PARAMETER;
	}
	else if (state == SWITCHSTATE_LIMIT)
	{
		printf("ERROR: Expected amount after `limit` switch.\n");
		return ERRORSEARCH_MISSING_PARAMETER;
	}

	return 0;
}

// If nonzero, bad parse.
static int parse_criteria(arg_parser_t *argparser, wadtool_options_search_t *options)
{
	switch(options->searchtype)
	{
		case ST_MAP:
		{
			options->criterion0 = takearg(argparser);
			if (!options->criterion0)
			{
				printf("ERROR: Expected map header entry name.\n");
				return ERRORSEARCH_MISSING_PARAMETER;
			}
			strupper(options->criterion0);
		}
		break;

		case ST_NAME:
		{
			options->criterion0 = takearg(argparser);
			if (!options->criterion0)
			{
				printf("ERROR: Expected name.\n");
				return ERRORSEARCH_MISSING_PARAMETER;
			}
			strupper(options->criterion0);
		}
		break;

		case ST_NAMESPACE:
		{
			options->criterion0 = takearg(argparser);
			if (!options->criterion0)
			{
				printf("ERROR: Expected namespace.\n");
				return ERRORSEARCH_MISSING_PARAMETER;
			}
			strupper(options->criterion0);
		}
		break;
	}
	return 0;
}

static int call(arg_parser_t *argparser)
{
	wadtool_options_search_t options = {NULL, NULL, 0, 0, 0, 0, &listentry_sort_index, ST_NONE, NULL, NULL, 0};

	int err;
	if (err = parse_mode(argparser, &options)) // the single equals is intentional.
	{
		return err;
	}
	if (err = parse_file(argparser, &options)) // the single equals is intentional.
	{
		return err;
	}
	if (err = parse_criteria(argparser, &options)) // the single equals is intentional.
	{
		WAD_Close(options.wad);
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
	printf("Usage: wad search [mode] [filename] ...\n");
	printf("\n");
	printf("       wad search maps [filename] [switches]\n");
	printf("       wad search map [filename] [headername] [switches]\n");
	printf("       wad search name [filename] [string] [switches]\n");
	printf("       wad search namespace [filename] [prefix] [switches]\n");
}

static void help()
{
	printf("[mode]: \n");
	printf("    The entry search mode.\n");
	printf("\n");
	printf("        map                 Finds entries belonging to a specific map.\n");
	printf("\n");
	printf("                            [headername]:\n");
	printf("                                The name of the map entry header.\n");
	printf("\n");
	printf("        maps                Finds all map header entries.\n");
	printf("\n");
	printf("        name                Finds all entries that start with a set of\n");
	printf("                            characters.\n");
	printf("\n");
	printf("                            [string]:\n");
	printf("                                The starting string for testing.\n");
	printf("\n");
	printf("        namespace           Finds all entries that are between XX_START\n");
	printf("                            and XX_END entries.\n");
	printf("\n");
	printf("                            [prefix]:\n");
	printf("                                The namespace characters (only up to two are\n");
	printf("                                used).\n");
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
	printf("\n");
	printf("        --limit x           Limits the amount of entries returned to `x`\n");
	printf("                            entries (after sorting).\n");
}

wadtool_t WADTOOL_Search = {
	"search",
	"Search a WAD for entries that fit certain criteria.",
	&call,
	&usage,
	&help,
};
