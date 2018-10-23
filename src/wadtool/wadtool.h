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

/**
 * Get the current argument.
 * @param argparser the parser to use.
 * @return the next argument, or NULL if no more.
 */
char* currarg(arg_parser_t *argparser);

/**
 * Check if the current argument is a certain string (case insensitive).
 * @param argparser the parser to use.
 * @param s the target string to compare.
 * @return 1 if equal, 0 if not.
 */
int currargis(arg_parser_t *argparser, char *s);

/**
 * Check if the current argument starts with a certain string (case insensitive).
 * @param argparser the parser to use.
 * @param s the target string to compare.
 * @return 1 if equal, 0 if not.
 */
int currargstart(arg_parser_t *argparser, char *s);

/**
 * Advances the parser and returns the argument.
 * @param argparser the parser to use.
 * @return the next argument, or NULL if no more.
 */
char* nextarg(arg_parser_t *argparser);

/**
 * Tests if the argument equals a string (case insensitive), and if it is, advance the parser.
 * @param argparser the parser to use.
 * @param s the string to test.
 * @return 1 if equal and the parser was advanced, 0 if not.
 */
int matcharg(arg_parser_t *argparser, char *s);

#endif
