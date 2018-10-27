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

#define WADTOOLS_SPLASH "WAD Tools v" WADTOOLS_VERSION " (C) 2018 Matt Tropiano"

// ==================== All Tools =====================

#include "wadtool/create.h"
#include "wadtool/info.h"
#include "wadtool/list.h"
#include "wadtool/search.h"

#define WADTOOL_COUNT 4
wadtool_t* WADTOOLS_ALL[WADTOOL_COUNT] = {
	&WADTOOL_Create,
	&WADTOOL_Info,
	&WADTOOL_List,
	&WADTOOL_Search,
};

// ================== Command Names ====================

#define COMMAND_HELP "help"
#define COMMAND_ALL "all"
#define COMMAND_CREATE "create"
#define COMMAND_INFO "info"
#define COMMAND_LIST "list"
#define COMMAND_SEARCH "search"

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
	printf("\n");
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
	else if (matcharg(argparser, COMMAND_CREATE))
		return &WADTOOL_Create;
	else if (matcharg(argparser, COMMAND_INFO))
		return &WADTOOL_Info;
	else if (matcharg(argparser, COMMAND_LIST))
		return &WADTOOL_List;
	else if (matcharg(argparser, COMMAND_SEARCH))
		return &WADTOOL_Search;
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
			print_help();
			printf("\n");
			int i;
			for (i = 0; i < WADTOOL_COUNT; i++)
			{
				wadtool_t* tool = WADTOOLS_ALL[i];
				printf("--------------------------------\n");
				printf("--- %s\n", tool->name);
				printf("--------------------------------\n");
				print_tool_help(tool);
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
