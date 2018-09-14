/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#include <stdio.h>
#include "lexer.h"

// ===========================================================================
// Common Private Functions
// ===========================================================================


// ===========================================================================
// Public Functions
// ===========================================================================

// ---------------------------------------------------------------
// lexer_t* LXR_Create(lexer_kernel_t *kernel)
// See lexer.h
// ---------------------------------------------------------------
lexer_t* LXR_Create(lexer_kernel_t *kernel);

// ---------------------------------------------------------------
// int LXR_Destroy(lexer_t *lexer)
// See lexer.h
// ---------------------------------------------------------------
int LXR_Destroy(lexer_t *lexer);

// ---------------------------------------------------------------
// int LXR_PushStream(lexer_t *lexer, char *filename)
// See lexer.h
// ---------------------------------------------------------------
int LXR_PushStream(lexer_t *lexer, char *filename);

// ---------------------------------------------------------------
// int LXR_PushStreamFile(lexer_t *lexer, char *name, FILE *file)
// See lexer.h
// ---------------------------------------------------------------
int LXR_PushStreamFile(lexer_t *lexer, char *name, FILE *file);

// ---------------------------------------------------------------
// int LXR_PushStreamBuffer(lexer_t *lexer, char *name, unsigned char *buffer, size_t length)
// See lexer.h
// ---------------------------------------------------------------
int LXR_PushStreamBuffer(lexer_t *lexer, char *name, unsigned char *buffer, size_t length);

// ---------------------------------------------------------------
// lexer_token_t* LXR_NextToken(lexer_t *lexer)
// See lexer.h
// ---------------------------------------------------------------
lexer_token_t* LXR_NextToken(lexer_t *lexer);

// TODO: Finish this.
