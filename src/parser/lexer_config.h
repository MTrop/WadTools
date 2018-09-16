/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#ifndef __LXR_CONFIG_H__
#define __LXR_CONFIG_H__

// Configuration for memory management in other systems.

#ifndef LXR_MALLOC
#define LXR_MALLOC(s)		malloc((s))
#endif

#ifndef LXR_FREE
#define LXR_FREE(s)			free((s))
#endif

#ifndef LXR_REALLOC
#define LXR_REALLOC(p,s)	realloc((p),(s))
#endif

#ifndef LXR_CALLOC
#define LXR_CALLOC(n,s)		calloc((n),(s))
#endif

#ifndef LEXEME_LENGTH_MAX
#define LEXEME_LENGTH_MAX	512
#endif

#ifndef LEXER_STREAM_BUFFER_SIZE
#define LEXER_STREAM_BUFFER_SIZE	32768
#endif

#endif
