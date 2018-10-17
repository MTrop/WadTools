/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#ifndef __WADTOOL_H__
#define __WADTOOL_H__

/** Argument parser. */
typedef struct {

    /** Arg count. */
    int argc;
    /** Arg vector. */
    char **argv;

    /** Arg index. */
    int index;
    /** Current argument. */
    char *arg;

} arg_parser_t;

/** Tool entry point. */
typedef struct {

    /** Name of command. */
    char* name;
    /** Description. */
    char* description;
    /** Calls the main bit of the tool (argc, argv are offset - argv[0] is the last token) */
    int (*call)(arg_parser_t*);
    /** Prints usage of the command. */
    void (*usage)();
    /** Prints help associated with the command. */
    void (*help)();

} wadtool_t;

int matcharg(arg_parser_t *argparser, char *t);

char* currarg(arg_parser_t *argparser);

/**
 * Advances the parser.
 * @param argparser the parser to use.
 */
char* nextarg(arg_parser_t *argparser);

#endif
