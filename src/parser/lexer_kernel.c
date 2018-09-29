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
#include <ctype.h>
#include "lexer_kernel.h"

// ===========================================================================
// Common Private Functions
// ===========================================================================

typedef struct {
	
	void *key;
	void *value;
	
} LXRK_SETPAIR;

static int LXRK_CompareChar(void *a, void *b)
{
	return (int)a - (int)b;
}

static int LXRK_CompareCharPtr(void *a, void *b)
{
	return strcmp((char*)a, (char*)b);
}

static int LXRK_ComparePairChar(void *a, void *b)
{
	LXRK_SETPAIR *pa = (LXRK_SETPAIR*)a;
	LXRK_SETPAIR *pb = (LXRK_SETPAIR*)b;
	return strcmp((char*)(pa->key), (char*)(pb->key));
}

static int LXRK_ComparePairCharPtr(void *a, void *b)
{
	LXRK_SETPAIR *pa = (LXRK_SETPAIR*)a;
	LXRK_SETPAIR *pb = (LXRK_SETPAIR*)b;
	return strcmp((char*)(pa->key), (char*)(pb->key));
}

static int LXRK_ComparePairCICharPtr(void *a, void *b)
{
	LXRK_SETPAIR *pa = (LXRK_SETPAIR*)a;
	LXRK_SETPAIR *pb = (LXRK_SETPAIR*)b;
	return stricmp((char*)(pa->key), (char*)(pb->key));
}

// Creates a new set pair.
static LXRK_SETPAIR* LXRK_CreatePair()
{
	return (LXRK_SETPAIR*)LXR_MALLOC(sizeof(LXRK_SETPAIR));
}

// Duplicates and allocates a string.
static char* LXRK_CreateString(char *orig)
{
	int len = strlen(orig);
	char *out = (char*)LXR_MALLOC(sizeof(char) * (strlen(orig)+1));
	strcpy(out, orig);
	out[len] = '\0';
	return out;
}

// Creates a new set pair.
static LXRK_SETPAIR* LXRK_CreateCharPtrCharPtrPair(char *key, char *value)
{
	LXRK_SETPAIR* pair = LXRK_CreatePair();
	pair->key = LXRK_CreateString(key);
	pair->value = LXRK_CreateString(value);
	return pair;
}

// Destroys a new set pair.
static void LXRK_DestroyCharPtrCharPtrPair(LXRK_SETPAIR *pair)
{
	LXR_FREE(pair->key);
	LXR_FREE(pair->value);
	LXR_FREE(pair);
}

// Creates a new set pair.
static LXRK_SETPAIR* LXRK_CreateCharPtrIntPair(char *key, int value)
{
	LXRK_SETPAIR* pair = LXRK_CreatePair();
	pair->key = LXRK_CreateString(key);
	pair->value = (void*)value;
	return pair;
}

// Destroys a new set pair.
static void LXRK_DestroyCharPtrIntPair(LXRK_SETPAIR *pair)
{
	LXR_FREE(pair->key);
	LXR_FREE(pair);
}

// Creates a new set pair.
static LXRK_SETPAIR* LXRK_CreateCharCharPair(int key, int value)
{
	LXRK_SETPAIR* pair = LXRK_CreatePair();
	pair->key = (void*)key;
	pair->value = (void*)value;
	return pair;
}

// Destroys a new set pair.
static void LXRK_DestroyCharCharPair(LXRK_SETPAIR *pair)
{
	LXR_FREE(pair);
}

static void LXRK_FreeSets(lexer_kernel_t *kernel)
{
	int i;
	if (kernel->comment_map) 
	{
		for (i = 0; i < kernel->comment_map->size; i++)
			LXRK_DestroyCharPtrCharPtrPair(kernel->comment_map->items[i]);
		LXR_FREE(kernel->comment_map);
	}
	if (kernel->comment_line_map)
	{
		for (i = 0; i < kernel->comment_line_map->size; i++)
			LXR_FREE(kernel->comment_line_map->items[i]);
		LXR_FREE(kernel->comment_line_map);
	}
	if (kernel->delimiter_map)
	{
		for (i = 0; i < kernel->delimiter_map->size; i++)
			LXRK_DestroyCharPtrIntPair(kernel->delimiter_map->items[i]);
		LXR_FREE(kernel->delimiter_map);
	}
	if (kernel->delimiter_starts) 
	{
		LXR_FREE(kernel->delimiter_starts);
	}
	if (kernel->end_comment_starts) 
	{
		LXR_FREE(kernel->end_comment_starts);
	}
	if (kernel->keyword_map)
	{
		for (i = 0; i < kernel->keyword_map->size; i++)
			LXRK_DestroyCharPtrIntPair(kernel->keyword_map->items[i]);
		LXR_FREE(kernel->keyword_map);
	}
	if (kernel->cikeyword_map) 
	{
		for (i = 0; i < kernel->cikeyword_map->size; i++)
			LXRK_DestroyCharPtrIntPair(kernel->cikeyword_map->items[i]);
		LXR_FREE(kernel->cikeyword_map);
	}
	if (kernel->string_map)
	{
		for (i = 0; i < kernel->string_map->size; i++)
			LXRK_DestroyCharCharPair(kernel->string_map->items[i]);
		LXR_FREE(kernel->string_map);
	}
}

static lexer_kernel_t* LXRK_Init()
{
	lexer_kernel_t *out = (lexer_kernel_t*)LXR_MALLOC(sizeof(lexer_kernel_t));
	
	out->comment_map = MT_SetCreate(16, &LXRK_ComparePairCharPtr);
	if (!out->comment_map)
		return NULL;
	out->comment_line_map = MT_SetCreate(16, &LXRK_CompareCharPtr);
	if (!out->comment_line_map)
		return NULL;
	out->delimiter_map = MT_SetCreate(16, &LXRK_ComparePairCharPtr);
	if (!out->delimiter_map)
		return NULL;
	out->delimiter_starts = MT_SetCreate(16, &LXRK_CompareChar);
	if (!out->delimiter_starts)
		return NULL;
	out->end_comment_starts = MT_SetCreate(16, &LXRK_CompareChar);
	if (!out->end_comment_starts)
		return NULL;
	out->keyword_map = MT_SetCreate(16, &LXRK_ComparePairCharPtr);
	if (!out->keyword_map)
		return NULL;
	out->cikeyword_map = MT_SetCreate(16, &LXRK_ComparePairCICharPtr);
	if (!out->cikeyword_map)
		return NULL;
	out->string_map = MT_SetCreate(16, &LXRK_ComparePairChar);
	if (!out->string_map)
		return NULL;
	out->decimal_char = '.';
	out->escape_char = '\\';
	
	return out;
}

// ===========================================================================
// Public Functions
// ===========================================================================

// ---------------------------------------------------------------
// lexer_kernel_t* LXRK_Create()
// See lexer_kernel.h
// ---------------------------------------------------------------
lexer_kernel_t* LXRK_Create()
{
	return LXRK_Init();
}

// ---------------------------------------------------------------
// int LXRK_Destroy(lexer_kernel_t *kernel)
// See lexer_kernel.h
// ---------------------------------------------------------------
int LXRK_Destroy(lexer_kernel_t *kernel)
{
	if (kernel == NULL)
		return 1;
	LXRK_FreeSets(kernel);
	LXR_FREE(kernel);
	return 0;
}

// ---------------------------------------------------------------
// void LXRK_AddCommentDelimiter(lexer_kernel_t *kernel, char *comment_start, char *comment_end)
// See lexer_kernel.h
// ---------------------------------------------------------------
void LXRK_AddCommentDelimiter(lexer_kernel_t *kernel, char *comment_start, char *comment_end)
{
	if (!comment_start || !strlen(comment_start) || !comment_end || !strlen(comment_end))
		return;
	MT_SetAdd(kernel->comment_map, LXRK_CreateCharPtrCharPtrPair(comment_start, comment_end));
	MT_SetAdd(kernel->delimiter_starts, (void*)(int)comment_start[0]);
	MT_SetAdd(kernel->end_comment_starts, (void*)(int)comment_end[0]);
}

// ---------------------------------------------------------------
// void LXRK_AddLineCommentDelimiter(lexer_kernel_t *kernel, char *delimiter)
// See lexer_kernel.h
// ---------------------------------------------------------------
void LXRK_AddLineCommentDelimiter(lexer_kernel_t *kernel, char *delimiter)
{
	if (!delimiter || !strlen(delimiter))
		return;
	MT_SetAdd(kernel->comment_line_map, LXRK_CreateString(delimiter));
	MT_SetAdd(kernel->delimiter_starts, (void*)(int)delimiter[0]);
}

// ---------------------------------------------------------------
// void LXRK_AddDelimiter(lexer_kernel_t *kernel, char *delimiter, int delimiter_type)
// See lexer_kernel.h
// ---------------------------------------------------------------
void LXRK_AddDelimiter(lexer_kernel_t *kernel, char *delimiter, int delimiter_type)
{
	if (!delimiter || !strlen(delimiter))
		return;
	MT_SetAdd(kernel->delimiter_map, LXRK_CreateCharPtrIntPair(delimiter, delimiter_type));
	MT_SetAdd(kernel->delimiter_starts, (void*)(int)delimiter[0]);
}

// ---------------------------------------------------------------
// void LXRK_AddKeyword(lexer_kernel_t *kernel, char *keyword, int keyword_type)
// See lexer_kernel.h
// ---------------------------------------------------------------
void LXRK_AddKeyword(lexer_kernel_t *kernel, char *keyword, int keyword_type)
{
	if (!keyword || !strlen(keyword))
		return;
	MT_SetAdd(kernel->keyword_map, LXRK_CreateCharPtrIntPair(keyword, keyword_type));
}

// ---------------------------------------------------------------
// void LXRK_AddCaseInsensitiveKeyword(lexer_kernel_t *kernel, char *keyword, int keyword_type)
// See lexer_kernel.h
// ---------------------------------------------------------------
void LXRK_AddCaseInsensitiveKeyword(lexer_kernel_t *kernel, char *keyword, int keyword_type)
{
	if (!keyword || !strlen(keyword))
		return;
	MT_SetAdd(kernel->cikeyword_map, LXRK_CreateCharPtrIntPair(keyword, keyword_type));
}

// ---------------------------------------------------------------
// void LXRK_AddStringDelimiters(lexer_kernel_t *kernel, char start, char end)
// See lexer_kernel.h
// ---------------------------------------------------------------
inline void LXRK_AddStringDelimiters(lexer_kernel_t *kernel, char start, char end)
{
	MT_SetAdd(kernel->string_map, LXRK_CreateCharCharPair(start, end));
}

// ---------------------------------------------------------------
// void LXRK_SetDecimalSeparator(lexer_kernel_t *kernel, char separator)
// See lexer_kernel.h
// ---------------------------------------------------------------
inline void LXRK_SetDecimalSeparator(lexer_kernel_t *kernel, char separator)
{
	kernel->decimal_char = separator;
}

// ---------------------------------------------------------------
// void LXRK_SetStringEscapeChar(lexer_kernel_t *kernel, char escape)
// See lexer_kernel.h
// ---------------------------------------------------------------
inline void LXRK_SetStringEscapeChar(lexer_kernel_t *kernel, char escape)
{
	kernel->escape_char = escape;
}

// ---------------------------------------------------------------
// char* LXRK_GetCommentEnd(lexer_kernel_t *kernel, char *comment_start)
// See lexer_kernel.h
// ---------------------------------------------------------------
char* LXRK_GetCommentEnd(lexer_kernel_t *kernel, char *comment_start)
{
	LXRK_SETPAIR pair;
	pair.key = (void*)comment_start;
	int idx;
	if ((idx = MT_SetSearch(kernel->comment_map, &pair)) >= 0)
		return (char*)(MT_SetGet(kernel->comment_map, LXRK_SETPAIR*, idx)->value);
	return NULL;
}

// ---------------------------------------------------------------
// int LXRK_IsLineComment(lexer_kernel_t *kernel, char *line_comment)
// See lexer_kernel.h
// ---------------------------------------------------------------
int LXRK_IsLineComment(lexer_kernel_t *kernel, char *line_comment)
{
	return MT_SetContains(kernel->comment_line_map, line_comment);
}

// ---------------------------------------------------------------
// char LXRK_GetStringEnd(lexer_kernel_t *kernel, char string_start)
// See lexer_kernel.h
// ---------------------------------------------------------------
char LXRK_GetStringEnd(lexer_kernel_t *kernel, char string_start)
{
	LXRK_SETPAIR pair;
	pair.key = (void*)(int)string_start;
	int idx;
	if ((idx = MT_SetSearch(kernel->string_map, &pair)) >= 0)
		return (char)(int)(MT_SetGet(kernel->string_map, LXRK_SETPAIR*, idx)->value);
	return 0;
}

// ---------------------------------------------------------------
// int LXRK_GetKeywordType(lexer_kernel_t *kernel, char* keyword)
// See lexer_kernel.h
// ---------------------------------------------------------------
int LXRK_GetKeywordType(lexer_kernel_t *kernel, char* keyword)
{
	LXRK_SETPAIR pair;
	pair.key = (void*)keyword;
	int idx;
	if ((idx = MT_SetSearch(kernel->keyword_map, &pair)) >= 0)
		return (int)(MT_SetGet(kernel->keyword_map, LXRK_SETPAIR*, idx)->value);
	else if ((idx = MT_SetSearch(kernel->cikeyword_map, &pair)) >= 0)
		return (int)(MT_SetGet(kernel->cikeyword_map, LXRK_SETPAIR*, idx)->value);
	return -1;
}

// ---------------------------------------------------------------
// int LXRK_GetDelimiterType(lexer_kernel_t *kernel, char* delimiter)
// See lexer_kernel.h
// ---------------------------------------------------------------
int LXRK_GetDelimiterType(lexer_kernel_t *kernel, char* delimiter)
{
	LXRK_SETPAIR pair;
	pair.key = (void*)delimiter;
	int idx;
	if ((idx = MT_SetSearch(kernel->delimiter_map, &pair)) >= 0)
		return (int)(MT_SetGet(kernel->delimiter_map, LXRK_SETPAIR*, idx)->value);
	return -1;
}

// ---------------------------------------------------------------
// int LXRK_IsAlphabeticalChar(lexer_kernel_t *kernel, int c)
// See lexer_kernel.h
// ---------------------------------------------------------------
inline int LXRK_IsAlphabeticalChar(lexer_kernel_t *kernel, int c)
{
	return isalpha(c);
}

// ---------------------------------------------------------------
// int LXRK_IsHexadecimalChar(lexer_kernel_t *kernel, int c)
// See lexer_kernel.h
// ---------------------------------------------------------------
inline int LXRK_IsHexadecimalChar(lexer_kernel_t *kernel, int c)
{
	return isxdigit(c);
}

// ---------------------------------------------------------------
// int LXRK_IsDecimalChar(lexer_kernel_t *kernel, int c)
// See lexer_kernel.h
// ---------------------------------------------------------------
inline int LXRK_IsDecimalChar(lexer_kernel_t *kernel, int c)
{
	return isdigit(c);
}

// ---------------------------------------------------------------
// int LXRK_IsUnderscoreChar(lexer_kernel_t *kernel, int c)
// See lexer_kernel.h
// ---------------------------------------------------------------
inline int LXRK_IsUnderscoreChar(lexer_kernel_t *kernel, int c)
{
	return c == '_' ? 1 : 0;
}

// ---------------------------------------------------------------
// int LXRK_IsNewlineChar(lexer_kernel_t *kernel, int c)
// See lexer_kernel.h
// ---------------------------------------------------------------
inline int LXRK_IsNewlineChar(lexer_kernel_t *kernel, int c)
{
	return c == '\n' ? 1 : 0;
}

// ---------------------------------------------------------------
// int LXRK_IsSpaceChar(lexer_kernel_t *kernel, int c)
// See lexer_kernel.h
// ---------------------------------------------------------------
inline int LXRK_IsSpaceChar(lexer_kernel_t *kernel, int c)
{
	return c == ' ' ? 1 : 0;
}

// ---------------------------------------------------------------
// int LXRK_IsTabChar(lexer_kernel_t *kernel, int c)
// See lexer_kernel.h
// ---------------------------------------------------------------
inline int LXRK_IsTabChar(lexer_kernel_t *kernel, int c)
{
	return c == '\t' ? 1 : 0;
}

// ---------------------------------------------------------------
// int LXRK_IsWhitespaceChar(lexer_kernel_t *kernel, int c)
// See lexer_kernel.h
// ---------------------------------------------------------------
inline int LXRK_IsWhitespaceChar(lexer_kernel_t *kernel, int c)
{
	return isspace(c);
}

// ---------------------------------------------------------------
// int LXRK_IsExponentSignChar(lexer_kernel_t *kernel, int c)
// See lexer_kernel.h
// ---------------------------------------------------------------
inline int LXRK_IsExponentSignChar(lexer_kernel_t *kernel, int c)
{
	return (c == 'e' || c == 'E') ? 1 : 0;
}

// ---------------------------------------------------------------
// int LXRK_IsDelimiterChar(lexer_kernel_t *kernel, int c)
// See lexer_kernel.h
// ---------------------------------------------------------------
inline int LXRK_IsDelimiterStartChar(lexer_kernel_t *kernel, int c)
{
	return MT_SetContains(kernel->delimiter_starts, (void*)c);
}

// ---------------------------------------------------------------
// int LXRK_IsEndCommentStartChar(lexer_kernel_t *kernel, int c)
// See lexer_kernel.h
// ---------------------------------------------------------------
inline int LXRK_IsEndCommentStartChar(lexer_kernel_t *kernel, int c)
{
	return MT_SetContains(kernel->end_comment_starts, (void*)c);
}

// ---------------------------------------------------------------
// int LXRK_IsStringStartChar(lexer_kernel_t *kernel, int c)
// See lexer_kernel.h
// ---------------------------------------------------------------
int LXRK_IsStringStartChar(lexer_kernel_t *kernel, int c)
{
	LXRK_SETPAIR pair;
	pair.key = (void*)c;
	return MT_SetContains(kernel->string_map, &pair);
}

// ---------------------------------------------------------------
// int LXRK_IsDecimalSeparatorChar(lexer_kernel_t *kernel, int c)
// See lexer_kernel.h
// ---------------------------------------------------------------
inline int LXRK_IsDecimalSeparatorChar(lexer_kernel_t *kernel, int c)
{
	return c == kernel->decimal_char ? 1 : 0;
}

// ---------------------------------------------------------------
// int LXRK_IsEscapeChar(lexer_kernel_t *kernel, int c)
// See lexer_kernel.h
// ---------------------------------------------------------------
inline int LXRK_IsEscapeChar(lexer_kernel_t *kernel, int c)
{
	return c == kernel->escape_char ? 1 : 0;
}
