/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#ifndef __STREAM_CONFIG_H__
#define __STREAM_CONFIG_H__

// Configuration for memory management in other systems.

#ifndef STREAM_MALLOC
#define STREAM_MALLOC(s)		malloc((s))
#endif

#ifndef STREAM_FREE
#define STREAM_FREE(s)			free((s))
#endif

#ifndef STREAM_REALLOC
#define STREAM_REALLOC(p,s)		realloc((p),(s))
#endif

#ifndef STREAM_CALLOC
#define STREAM_CALLOC(n,s)		calloc((n),(s))
#endif

#endif