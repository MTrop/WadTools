/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#ifndef __WADSTREAM_H__
#define __WADSTREAM_H__

#include "wad/wad.h"
#include "io/stream.h"

/**
 * Opens a stream for reading from a WAD file's contents.
 * Stream is either buffer-based or file-based depending on the type of WAD implementation.
 * See io/stream.h for more info.
 * @param wad the open WAD file.
 * @param entry the WAD entry to use.
 * @return a pointer to a stream (that must be closed with STREAM_Close), or NULL if wad is invalid or other errors.
 */
stream_t* STREAM_OpenWADStream(wad_t *wad, wadentry_t *entry);

#endif
