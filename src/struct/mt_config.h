/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#ifndef __MT_CONFIG_H__
#define __MT_CONFIG_H__

// Configuration for memory management in other systems.

#ifndef MTS_MALLOC
#define MTS_MALLOC(s)		malloc((s))
#endif

#ifndef MTS_FREE
#define MTS_FREE(s)			free((s))
#endif

#ifndef MTS_REALLOC
#define MTS_REALLOC(p,s)	realloc((p),(s))
#endif

#ifndef MTS_CALLOC
#define MTS_CALLOC(n,s)		calloc((n),(s))
#endif

#endif
