/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer_kernel.h"

// ===========================================================================
// Common Private Functions
// ===========================================================================

typedef struct {
	void *key;
	void *value;
} LXR_SETPAIR;

static int LXR_CompareChar(void *a, void *b)
{
	return (int)a - (int)b;
}

static int LXR_CompareCharPtr(void *a, void *b)
{
	return strcmp((char*)a, (char*)b);
}

static int LXR_ComparePairChar(void *a, void *b)
{
	LXR_SETPAIR *pa = (LXR_SETPAIR*)a;
	LXR_SETPAIR *pb = (LXR_SETPAIR*)b;
	return strcmp((char*)(pa->key), (char*)(pb->key));
}

static int LXR_ComparePairCharPtr(void *a, void *b)
{
	LXR_SETPAIR *pa = (LXR_SETPAIR*)a;
	LXR_SETPAIR *pb = (LXR_SETPAIR*)b;
	return strcmp((char*)(pa->key), (char*)(pb->key));
}

static int LXR_ComparePairCICharPtr(void *a, void *b)
{
	LXR_SETPAIR *pa = (LXR_SETPAIR*)a;
	LXR_SETPAIR *pb = (LXR_SETPAIR*)b;
	return stricmp((char*)(pa->key), (char*)(pb->key));
}

static lexer_kernel_t* LXR_KernelInit()
{
	lexer_kernel_t *out = (lexer_kernel_t*)LXR_MALLOC(sizeof(lexer_kernel_t));
	
	out->comment_map = MT_SetCreate(16, &LXR_ComparePairCharPtr);
	out->comment_line_map = MT_SetCreate(16, &LXR_CompareCharPtr);
	out->delimiter_map = MT_SetCreate(16, &LXR_ComparePairCharPtr);
	out->delimiter_starts = MT_SetCreate(16, &LXR_CompareChar);
	out->keyword_map = MT_SetCreate(16, &LXR_ComparePairCharPtr);
	out->cikeyword_map = MT_SetCreate(16, &LXR_ComparePairCICharPtr);
	out->string_map = MT_SetCreate(16, &LXR_ComparePairChar);
	out->decimal_char = '.';
	out->escape_char = '\\';
	
	return out;
}


// ===========================================================================
// Public Functions
// ===========================================================================

// ---------------------------------------------------------------
// lexer_kernel_t* LXR_KernelCreate()
// See lexer_kernel.h
// ---------------------------------------------------------------
lexer_kernel_t* LXR_KernelCreate();

// ---------------------------------------------------------------
// void LXR_KernelAddCommentDelimiter(lexer_kernel_t *kernel, char *comment_start, char *comment_end)
// See lexer_kernel.h
// ---------------------------------------------------------------
void LXR_KernelAddCommentDelimiter(lexer_kernel_t *kernel, char *comment_start, char *comment_end);

// ---------------------------------------------------------------
// void LXR_KernelAddLineCommentDelimiter(lexer_kernel_t *kernel, char *delimiter)
// See lexer_kernel.h
// ---------------------------------------------------------------
void LXR_KernelAddLineCommentDelimiter(lexer_kernel_t *kernel, char *delimiter);

// ---------------------------------------------------------------
// lexer_kernel_t* LXR_KernelCreate()
// See lexer_kernel.h
// ---------------------------------------------------------------
void LXR_KernelAddDelimiter(lexer_kernel_t *kernel, char *delimiter, int delimiter_type);

// ---------------------------------------------------------------
// void LXR_KernelAddKeyword(lexer_kernel_t *kernel, char *keyword, int keyword_type)
// See lexer_kernel.h
// ---------------------------------------------------------------
void LXR_KernelAddKeyword(lexer_kernel_t *kernel, char *keyword, int keyword_type);

// ---------------------------------------------------------------
// void LXR_KernelAddCaseInsensitiveKeyword(lexer_kernel_t *kernel, char *keyword, int keyword_type)
// See lexer_kernel.h
// ---------------------------------------------------------------
void LXR_KernelAddCaseInsensitiveKeyword(lexer_kernel_t *kernel, char *keyword, int keyword_type);

// ---------------------------------------------------------------
// void LXR_KernelAddStringDelimiters(lexer_kernel_t *kernel, char start, char end)
// See lexer_kernel.h
// ---------------------------------------------------------------
void LXR_KernelAddStringDelimiters(lexer_kernel_t *kernel, char start, char end);

// ---------------------------------------------------------------
// void LXR_SetDecimalSeparator(lexer_kernel_t *kernel, char separator)
// See lexer_kernel.h
// ---------------------------------------------------------------
void LXR_SetDecimalSeparator(lexer_kernel_t *kernel, char separator);

// ---------------------------------------------------------------
// void LXR_SetStringEscapeChar(lexer_kernel_t *kernel, char escape)
// See lexer_kernel.h
// ---------------------------------------------------------------
void LXR_SetStringEscapeChar(lexer_kernel_t *kernel, char escape);

// ---------------------------------------------------------------
// int LXR_KernelDestroy(lexer_kernel_t *kernel)
// See lexer_kernel.h
// ---------------------------------------------------------------
int LXR_KernelDestroy(lexer_kernel_t *kernel);
