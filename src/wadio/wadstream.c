/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "wadstream.h"

// ---------------------------------------------------------------
// stream_t* STREAM_OpenWADStream(wad_t *wad, wadentry_t *entry)
// See wadstream.h
// ---------------------------------------------------------------
stream_t* STREAM_OpenWADStream(wad_t *wad, wadentry_t *entry)
{
	if (!wad)
		return NULL;

	switch (wad->type)
	{
		default:
		case WI_MAP:
			return NULL;
		case WI_FILE:
		{
			FILE *fp = wad->handle.file;
			fseek(fp, entry->offset, SEEK_SET);
			return STREAM_OpenBufferedFileSection(fp, entry->length, 16384);
		}
		case WI_BUFFER:
		{
			unsigned char *buf = wad->handle.buffer;
			return STREAM_OpenBuffer(buf + (entry->offset - sizeof(wadheader_t)), entry->length);
		}
	}
}
