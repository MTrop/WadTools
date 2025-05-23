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

#define WADTOOLS_VERSION "0.1.0"

#define WADTOOLS_SPLASH "WAD Tools v" WADTOOLS_VERSION " (C) 2018-2025 Matt Tropiano"

// ==================== All Tools =====================

#include "wadtool/create.h"
#include "wadtool/info.h"
#include "wadtool/list.h"
#include "wadtool/search.h"
#include "wadtool/dump.h"
#include "wadtool/shift.h"
#include "wadtool/swap.h"
#include "wadtool/rename.h"
#include "wadtool/add.h"
#include "wadtool/remove.h"
#include "wadtool/marker.h"
#include "wadtool/clean.h"

#define WADTOOL_COUNT 12
wadtool_t* WADTOOLS_ALL[WADTOOL_COUNT] = {
	&WADTOOL_Add,
	&WADTOOL_Clean,
	&WADTOOL_Create,
	&WADTOOL_Dump,
	&WADTOOL_Info,
	&WADTOOL_List,
	&WADTOOL_Marker,
	&WADTOOL_Rename,
	&WADTOOL_Remove,
	&WADTOOL_Search,
	&WADTOOL_Shift,
	&WADTOOL_Swap,
};

// ================== Command Names ====================

#define COMMAND_HELP 	"help"
#define COMMAND_ALL 	"all"

#define COMMAND_ADD 	"add"
#define COMMAND_CLEAN	"clean"
#define COMMAND_CREATE	"create"
#define COMMAND_DUMP 	"dump"
#define COMMAND_INFO 	"info"
#define COMMAND_LIST 	"list"
#define COMMAND_MARKER 	"marker"
#define COMMAND_REMOVE 	"remove"
#define COMMAND_RENAME 	"rename"
#define COMMAND_SEARCH 	"search"
#define COMMAND_SHIFT 	"shift"
#define COMMAND_SWAP 	"swap"

// =====================================================


static void print_usage()
{
	printf("Usage: wad [command] [arguments]\n");
	printf("       wad help [command]\n");
	printf("       wad help all\n");
}

static void print_help()
{
	int i;
	printf("Available commands:");
	printf("\n");
	for (i = 0; i < WADTOOL_COUNT; i++)
		printf("    %-16s %s\n", WADTOOLS_ALL[i]->name, WADTOOLS_ALL[i]->description);
}

static int print_splash(arg_parser_t *argparser)
{
	printf(WADTOOLS_SPLASH "\n");
	print_usage();
	return 0;
}

wadtool_t DEFAULT_TOOL =
{
	"DEFAULT",
	WADTOOLS_SPLASH,
	&print_splash,
	&print_usage,
	&print_help,
};

// Parses the tool from the arguments.
static wadtool_t* parse_tool(arg_parser_t *argparser)
{
	if (!argparser->arg)
		return &DEFAULT_TOOL;
	else if (matcharg(argparser, COMMAND_ADD))
		return &WADTOOL_Add;
	else if (matcharg(argparser, COMMAND_CLEAN))
		return &WADTOOL_Clean;
	else if (matcharg(argparser, COMMAND_CREATE))
		return &WADTOOL_Create;
	else if (matcharg(argparser, COMMAND_INFO))
		return &WADTOOL_Info;
	else if (matcharg(argparser, COMMAND_LIST))
		return &WADTOOL_List;
	else if (matcharg(argparser, COMMAND_MARKER))
		return &WADTOOL_Marker;
	else if (matcharg(argparser, COMMAND_SEARCH))
		return &WADTOOL_Search;
	else if (matcharg(argparser, COMMAND_DUMP))
		return &WADTOOL_Dump;
	else if (matcharg(argparser, COMMAND_SHIFT))
		return &WADTOOL_Shift;
	else if (matcharg(argparser, COMMAND_SWAP))
		return &WADTOOL_Swap;
	else if (matcharg(argparser, COMMAND_RENAME))
		return &WADTOOL_Rename;
	else if (matcharg(argparser, COMMAND_REMOVE))
		return &WADTOOL_Remove;
	else
		return &DEFAULT_TOOL;
}

void print_tool_help(wadtool_t* tool)
{
	printf("%s\n", tool->description);
	(tool->usage)();
	printf("\n");
	(tool->help)();
	printf("\n");
}

/**
 * Main entry point.
 */
int main(int argc, char **argv)
{
	arg_parser_t parser = {argc, argv, 0, NULL};
	
	// consume the command name.
	nextarg(&parser);

	// prime parse
	nextarg(&parser);
	if (matcharg(&parser, COMMAND_HELP))
	{
		if (matcharg(&parser, COMMAND_ALL))
		{
			print_splash(&parser);
			printf("\n");
			print_help();
			printf("\n");
			int i;
			for (i = 0; i < WADTOOL_COUNT; i++)
			{
				wadtool_t* tool = WADTOOLS_ALL[i];
				printf("-------------------------------------------------------------------------------\n");
				printf("--- %s\n", tool->name);
				printf("-------------------------------------------------------------------------------\n");
				print_tool_help(tool);
				printf("\n");
			}
			return 0;
		}
		else
		{
			print_tool_help(parse_tool(&parser));
			return 0;
		}
	}
	else
	{
		wadtool_t* tool = parse_tool(&parser);
		return (tool->call)(&parser);
	}
}
