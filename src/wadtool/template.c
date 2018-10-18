/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#if 0 // Remove this.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "wadtool.h"
#include "../wad/wad.h"
#include "../wad/waderrno.h"

extern int errno;
extern int waderrno;

#define ERRORTEMPLATE_NONE        0

typedef struct
{

} wadtool_options_template_t;


static int exec(wadtool_options_template_t *options)
{
    // TODO: Finish this.
    return ERRORTEMPLATE_NONE;
}

static int call(arg_parser_t *argparser)
{
    wadtool_options_template_t options = {};

    // TODO: Setup.
    
    int ret = exec(&options);
    
    // TODO: Cleanup.
    
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
    "name",
    "Description",
    &call,
    &usage,
    &help,
};

#endif // Remove this