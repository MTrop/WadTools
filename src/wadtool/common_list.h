/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#ifndef __WADTOOL_LIST_COMMON_H__
#define __WADTOOL_LIST_COMMON_H__

#include "wad/wad.h"

#define SWITCH_PREFIX			"-"

#define SWITCH_INDICES			"--indices"
#define SWITCH_INDICES2			"-i"
#define SWITCH_NAMES			"--names"
#define SWITCH_NAMES2			"-n"
#define SWITCH_LENGTHS			"--lengths"
#define SWITCH_LENGTHS2			"-l"
#define SWITCH_OFFSETS			"--offsets"
#define SWITCH_OFFSETS2			"-o"
#define SWITCH_ALL			    "--all"

#define SWITCH_NOHEADER		    "--no-header"
#define SWITCH_NOHEADER2	    "-nh"
#define SWITCH_INLINEHEADER	    "--inline-header"
#define SWITCH_INLINEHEADER2    "-ih"

#define SWITCH_SORT				"--sort"
#define SWITCH_SORT2		    "-s"
#define SWITCH_REVERSESORT		"--reverse-sort"
#define SWITCH_REVERSESORT2	    "-rs"
#define SWITCH_LIMIT			"--count"
#define SWITCH_LIMIT2			"-c"

#define SORT_INDEX				"index"
#define SORT_NAME				"name"
#define SORT_LENGTH				"length"
#define SORT_OFFSET				"offset"

#endif