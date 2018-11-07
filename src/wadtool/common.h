/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#ifndef __WADTOOL_COMMON_H__
#define __WADTOOL_COMMON_H__

#include "wad/wad.h"

/** Enum for entry type resolver. */
typedef enum
{
	ET_DETECT,
	ET_INDEX,
	ET_NAME,

} entrytype_t;

/**
 * Searches for an entry index using a string input,
 * interpreted as either numeric or string depending on entrytype.
 * @param wad the wad to search in.
 * @param entrytype the entry search type.
 * @param entry the input name/index as a string.
 * @param start the starting offset for search.
 * @return the corresponding/parsed index or -1 if not found.
 */
int WADTools_FindEntryIndex(wad_t *wad, entrytype_t entrytype, const char *entry, int start);

#endif
