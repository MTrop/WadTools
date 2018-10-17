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

#define SWITCH_MINIMAL          "-m"
#define SWITCH_MINIMAL2         "--minimal"

typedef struct
{
    int minimal;

} wadtool_info_options_t;

static void print_info(wad_t* wad, wadtool_info_options_t* options, char *filename)
{
    if (options->minimal)
    {
        printf("%s ", filename);
        printf("%s ", wad->header.type == WADTYPE_IWAD ? "IWAD" : "PWAD");
        printf("%d ", wad->header.entry_count);
        printf("%d ", wad->header.entry_list_offset - sizeof(wadheader_t));
        printf("%d ", wad->header.entry_list_offset);
        printf("%d\n", (wad->header.entry_list_offset + (sizeof(wadentry_t) * wad->header.entry_count)));
    }
    else
    {
        printf("%s: %s, ", filename, wad->header.type == WADTYPE_IWAD ? "IWAD" : "PWAD");
        printf("%d entries, ", wad->header.entry_count);
        printf("%d bytes content, ", wad->header.entry_list_offset - sizeof(wadheader_t));
        printf("list at byte %d, ", wad->header.entry_list_offset);
        printf("%d bytes\n", (wad->header.entry_list_offset + (sizeof(wadentry_t) * wad->header.entry_count)));
    }
}

static int call(arg_parser_t *argparser)
{
    char *filename = currarg(argparser);
    if (!filename)
    {
        fprintf(stderr, "ERROR: No WAD file.\n");
        return ERRORINFO_NO_FILENAME;
    }

    // Open a shallow mapping.
	wad_t *wad = WAD_OpenMap(filename);
	
	if (!wad)
	{
		if (waderrno == WADERROR_FILE_ERROR)
			printf("ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
		else
			printf("ERROR: %s\n", strwaderror(waderrno));
		return ERRORINFO_WAD_ERROR + waderrno;
	}
	
    wadtool_info_options_t options = {0};

    char *s = nextarg(argparser);
    if (matcharg(argparser, SWITCH_MINIMAL))
        options.minimal = 1;
    else if (matcharg(argparser, SWITCH_MINIMAL2))
        options.minimal = 1;
    else if (s)
    {
        printf("ERROR: Bad switch: %s\n", s);
        WAD_Close(wad);
        return ERRORINFO_BAD_SWITCH;
    }


    print_info(wad, &options, filename);

	WAD_Close(wad);

    return ERRORINFO_NONE;
}

static void usage()
{
    printf("Usage: wad info [filename] [switches]\n");
}

static void help()
{
    printf("[filename]: \n");
    printf("        The name of the WAD file to inspect.\n");
    printf("\n");
    printf("[switches]: \n");
    printf("        --minimal      Print information minimally.\n");
    printf("        -m\n");
}

wadtool_t WADTOOL_Info = {
    "info",
    "Displays basic information on a WAD file.",
    &call,
    &usage,
    &help,
};
