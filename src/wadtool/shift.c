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
#include "wad/waderrno.h"

extern int errno;
extern int waderrno;

#define ERRORTEMPLATE_NONE        0
#define ERRORTEMPLATE_NO_FILENAME 1
#define ERRORTEMPLATE_WAD_ERROR   10

typedef struct
{
	/** WAD filename. */
	char *filename;
	/** The WAD to use. */
	wad_t *wad;

	/** Entry start. */
	int start;
	/** Entry end. */
	int end;
	/** DEstination index. */
	int destination;

} wadtool_options_shift_t;


static int exec(wadtool_options_shift_t *options)
{
	printf("ERROR: NOT SUPPORTED YET!\n");
	return -1;
}

// If nonzero, bad parse.
static int parse_file(arg_parser_t *argparser, wadtool_options_shift_t *options)
{
	options->filename = currarg(argparser);
	if (!options->filename)
	{
		fprintf(stderr, "ERROR: No WAD file.\n");
		return ERRORTEMPLATE_NO_FILENAME;
	}

	// Open a file.
	options->wad = WAD_Open(options->filename);

	if (!options->wad)
	{
		if (waderrno == WADERROR_FILE_ERROR)
			fprintf(stderr, "ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
		else
			fprintf(stderr, "ERROR: %s\n", strwaderror(waderrno));
		return ERRORTEMPLATE_WAD_ERROR + waderrno;
	}
	nextarg(argparser);
	return 0;
}

#define SWITCHSTATE_INIT		0

// If nonzero, bad parse.
static int parse_switches(arg_parser_t *argparser, wadtool_options_shift_t *options)
{
	int state = SWITCHSTATE_INIT;
	return 0;
}

static int call(arg_parser_t *argparser)
{
	wadtool_options_shift_t options = {NULL, NULL, -1, -1, -1};

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
	// TODO: Finish this.
	printf("Usage: wad template\n");
}

static void help()
{
	// TODO: Finish this.
}

wadtool_t WADTOOL_Template = {
	"shift",
	"Shifts the order of one or more entries.",
	&call,
	&usage,
	&help,
};
