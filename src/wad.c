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

#define WADTOOL_COUNT 2
wadtool_t* WADTOOLS_ALL[WADTOOL_COUNT] = {
    &WADTOOL_Create,
    &WADTOOL_Info,
};

// ================== Command Names ====================

#define COMMAND_HELP "help"
#define COMMAND_CREATE "create"
#define COMMAND_INFO "info"

// =====================================================


static void print_usage()
{
    printf("Usage: wad [command] [arguments]\n");
    printf("       wad help [command]\n");
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
    else
        return &DEFAULT_TOOL;
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
        wadtool_t* tool = parse_tool(&parser);
        printf("%s\n", tool->description);
        (tool->usage)();
        (tool->help)();
        return 0;
    }
    else
    {
        wadtool_t* tool = parse_tool(&parser);
        return (tool->call)(&parser);
    }
}
