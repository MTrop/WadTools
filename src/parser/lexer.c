/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lexer_config.h"
#include "lexer.h"
#include "../io/stream.h"

#define LXRC_END_OF_LEXER	-1
#define LXRC_END_OF_STREAM	-2


// ===========================================================================
// Common Private Functions
// ===========================================================================

// Resets a lexer token.
static void LXR_ResetToken(lexer_token_t *token)
{
	token->stream_name = NULL;
	token->type = LXRT_UNKNOWN;
	token->subtype = -1;
	token->lexeme[0] = '\0';
	token->length = 0;
	token->line_number = 0;
}

// Resets lexer options.
static void LXR_ResetOptions(lexer_options_t *options)
{
	options->include_spaces = 0;
	options->include_tabs = 0;
	options->include_newlines = 0;
	options->include_stream_break = 0;
}

// Creates a lexer stream.
static lexer_stream_t* LXR_CreateStream(char *name, stream_t *stream)
{
	lexer_stream_t *out = (lexer_stream_t*)LXR_MALLOC(sizeof(lexer_stream_t));
	if (!out)
		return NULL;
	out->name = name;
	out->stream = stream;
	out->line_number = 1;
	out->character_number = 0;
	return out;
}

// Pushes a new lexer stream node onto the stack.
static int LXR_PushStreamNode(lexer_t *lexer, char *name, stream_t *stream)
{
	lexer_stream_stack_t *lxrssnode = (lexer_stream_stack_t*)LXR_MALLOC(sizeof(lexer_stream_stack_t));
	if (!lxrssnode)
		return 1;
	
	lexer_stream_t *lxrs = LXR_CreateStream(name, stream);
	if (!lxrs)
	{
		LXR_FREE(lxrssnode);
		return 1;
	}

	lxrssnode->previous = lexer->stream_stack;
	lxrssnode->stream = lxrs;
	lexer->stream_stack = lxrssnode;
	
	return 0;
}

// Pops and destroys a lexer stream node.
static stream_t* LXR_PopStreamNode(lexer_t *lexer)
{
	// underrun test.
	if (!lexer->stream_stack)
		return NULL;
	
	lexer_stream_stack_t *node = lexer->stream_stack;
	lexer_stream_t *lxrs = node->stream;
	lexer->stream_stack = node->previous;
	LXR_FREE(node);
	
	stream_t *out = lxrs->stream;
	LXR_FREE(lxrs);
	
	return out;
}

// Get a single character from a lexer.
// Ignores CR.
// Return LXRC_END_OF_LEXER if no more streams.
// Return LXRC_END_OF_STREAM if end of stream.
static int LXR_GetChar(lexer_t *lexer)
{
	if (!lexer->stream_stack)
		return LXRC_END_OF_LEXER;

	int out;
	lexer_stream_t *lexerstream = lexer->stream_stack->stream;
	
	// skip CR
	do {
		out = STREAM_GetChar(lexerstream->stream);
		if (out == 0x0A) // line feed
		{
			lexerstream->line_number++;
			lexerstream->character_number = 0;
		}
		else if (out != 0x0D) // CR (ignored)
		{
			lexerstream->character_number++;
		}
	} while (out == 0x0D);
		
	if (out < 0)
		return LXRC_END_OF_STREAM;
	
	return out;
}

// Adds to the current token.
static void LXR_AddToToken(lexer_t *lexer, int c)
{
	if (lexer->token.length >= LEXEME_LENGTH_MAX - 1)
		return;
	
	int next = (lexer->token.length)++;
	lexer->token.lexeme[next] = c & 0x0FF;
	lexer->token.lexeme[lexer->token.length] = 0x00; // null-terminate

	// save starting line
	lexer->token.line_number = lexer->stream_stack->stream->line_number;
}

// Adds to the current token, but does not increase length - this for lookup lookahead.
static void LXR_AddToTokenTemp(lexer_t *lexer, int c)
{
	if (lexer->token.length >= LEXEME_LENGTH_MAX - 1)
		return;
	
	lexer->token.lexeme[lexer->token.length] = c & 0x0FF;
	lexer->token.lexeme[lexer->token.length+1] = 0x00; // null-terminate
}

// Flatten token back to current length.
static void LXR_FlattenToken(lexer_t *lexer)
{
	lexer->token.lexeme[lexer->token.length] = '\0';
}

// Finishes the current token.
static void LXR_FinishToken(lexer_t *lexer, lexeme_type_t state)
{
	LXR_FlattenToken(lexer);
	lexer->token.stream_name = lexer->stream_stack->stream->name;
	
	lexer->token.type = state;
	lexer->token.subtype = -1;

	if (lexer->token.type == LXRT_NUMBER)
	{
		char *t;
		if (strchr(lexer->token.lexeme, '.'))
			lexer->token.subtype = LXRTN_FLOAT;
		else if ((t = strstr(lexer->token.lexeme, "0x")) && t == lexer->token.lexeme)
			lexer->token.subtype = LXRTN_HEX;
		else if ((t = strstr(lexer->token.lexeme, "0X")) && t == lexer->token.lexeme)
			lexer->token.subtype = LXRTN_HEX;
		else if ((t = strstr(lexer->token.lexeme, "0")) && t == lexer->token.lexeme)
			lexer->token.subtype = LXRTN_OCTAL;
		else if (strchr(lexer->token.lexeme, 'e'))
			lexer->token.subtype = LXRTN_FLOAT;
		else if (strchr(lexer->token.lexeme, 'E'))
			lexer->token.subtype = LXRTN_FLOAT;
		else
			lexer->token.subtype = LXRTN_INTEGER;
	}
	else if (lexer->token.type == LXRT_IDENTIFIER)
	{
		lexer->token.subtype = LXRK_GetKeywordType(lexer->kernel, lexer->token.lexeme);
		if (lexer->token.subtype >= 0)
			lexer->token.type = LXRT_KEYWORD;
	}
	else if (lexer->token.type == LXRT_DELIMITER)
		lexer->token.subtype = LXRK_GetDelimiterType(lexer->kernel, lexer->token.lexeme);
	
}

// ===========================================================================
// Public Functions
// ===========================================================================

static char *tokentypename[LXRT_COUNT] = {
	"END_OF_LEXER",
	"END_OF_STREAM",
	"UNKNOWN",
	"ILLEGAL",
	"SPACE",
	"TAB",
	"NEWLINE",
	"IDENTIFIER",
	"KEYWORD",
	"DELIMITER",
	"NUMBER",
	"STRING",
	"STATE_POINT",
	"STATE_FLOAT",
	"STATE_COMMENT",
	"STATE_END_COMMENT",
	"STATE_LINE_COMMENT",
	"STATE_HEX_INTEGER0",
	"STATE_HEX_INTEGER1",
	"STATE_HEX_INTEGER",
	"STATE_EXPONENT",
	"STATE_EXPONENT_POWER",
};

static char *tokennumerictypename[LXRTN_COUNT] = {
	"INTEGER",
	"FLOAT",
	"OCTAL",
	"HEX",
};

// ---------------------------------------------------------------
// char* LXR_TokenTypeName(lexeme_type_t type)
// See lexer.h
// ---------------------------------------------------------------
char* LXR_TokenTypeName(lexeme_type_t type)
{
	if (type < 0 || type >= LXRT_COUNT)
		return NULL;
	else
		return tokentypename[type];
}

// ---------------------------------------------------------------
// char* LXR_TokenNumericSubtypeName(lexeme_numeric_subtype_t subtype)
// See lexer.h
// ---------------------------------------------------------------
char* LXR_TokenNumericSubtypeName(lexeme_numeric_subtype_t subtype)
{
	if (subtype < 0 || subtype >= LXRTN_COUNT)
		return NULL;
	else
		return tokennumerictypename[subtype];
}

// ---------------------------------------------------------------
// lexer_t* LXR_Create(lexer_kernel_t *kernel)
// See lexer.h
// ---------------------------------------------------------------
lexer_t* LXR_Create(lexer_kernel_t *kernel)
{
	lexer_t *out = (lexer_t*)LXR_MALLOC(sizeof(lexer_t));
	if (!out)
		return NULL;
		
	out->kernel = kernel;
	out->stream_stack = NULL;
	out->state = LXRT_UNKNOWN;
	out->string_end = '\0';
	out->comment_end = NULL;
	out->stored = '\0';
	
	LXR_ResetOptions(&(out->options));
	LXR_ResetToken(&(out->token));

	return out;
}

// ---------------------------------------------------------------
// int LXR_Destroy(lexer_t *lexer)
// See lexer.h
// ---------------------------------------------------------------
int LXR_Destroy(lexer_t *lexer)
{
	if (!lexer)
		return 1;
	
	while (lexer->stream_stack)
		LXR_PopStream(lexer);
	
	LXR_FREE(lexer);
	return 0;
}

// ---------------------------------------------------------------
// int LXR_PushStream(lexer_t *lexer, char *filename)
// See lexer.h
// ---------------------------------------------------------------
int LXR_PushStream(lexer_t *lexer, char *filename)
{
	stream_t *stream = STREAM_OpenBuffered(filename, LEXER_STREAM_BUFFER_SIZE);
	if (!stream)
		return 1;
	
	return LXR_PushStreamNode(lexer, filename, stream);
}

// ---------------------------------------------------------------
// int LXR_PushStreamFile(lexer_t *lexer, char *name, FILE *file)
// See lexer.h
// ---------------------------------------------------------------
int LXR_PushStreamFile(lexer_t *lexer, char *name, FILE *file)
{
	stream_t *stream = STREAM_OpenBufferedFile(file, LEXER_STREAM_BUFFER_SIZE);
	if (!stream)
		return 1;
	
	return LXR_PushStreamNode(lexer, name, stream);	
}

// ---------------------------------------------------------------
// int LXR_PushStreamBuffer(lexer_t *lexer, char *name, unsigned char *buffer, size_t length)
// See lexer.h
// ---------------------------------------------------------------
int LXR_PushStreamBuffer(lexer_t *lexer, char *name, unsigned char *buffer, size_t length)
{
	stream_t *stream = STREAM_OpenBuffer(buffer, length);
	if (!stream)
		return 1;
	
	return LXR_PushStreamNode(lexer, name, stream);	
}

// ---------------------------------------------------------------
// int LXR_PopStream(lexer_t *lexer)
// See lexer.h
// ---------------------------------------------------------------
int LXR_PopStream(lexer_t *lexer)
{
	stream_t *stream = LXR_PopStreamNode(lexer);
	if (!stream)
		return 1;
	
	STREAM_Close(stream);
	return 0;
}

// ---------------------------------------------------------------
// lexer_token_t* LXR_NextToken(lexer_t *lexer)
// See lexer.h
// ---------------------------------------------------------------
lexer_token_t* LXR_NextToken(lexer_t *lexer)
{
	LXR_ResetToken(&(lexer->token));
	lexeme_type_t state = LXRT_UNKNOWN;
	
	int c, i, h;
	int breakloop = 0;
	while (!breakloop)
	{
		// read character.
		if (lexer->stored)
		{
			c = lexer->stored;
			lexer->stored = 0;
		}
		else
		{
			c = LXR_GetChar(lexer);
		}
		
		switch (state)
		{
			case LXRT_END_OF_LEXER:
			{
				breakloop = 1;
			}
			break;

			case LXRT_UNKNOWN:
			{
				if (c == LXRC_END_OF_LEXER) // Lexer End
				{
					state = LXRT_END_OF_LEXER;
					breakloop = 1;
				}
				else if (c == LXRC_END_OF_STREAM) // Stream End
				{
					if (lexer->options.include_stream_break)
					{
						state = LXRT_END_OF_STREAM;
						breakloop = 1;
					}
					LXR_PopStream(lexer);
				}
				else if (LXRK_IsNewlineChar(lexer->kernel, c))
				{
					if (lexer->options.include_newlines)
					{
						state = LXRT_NEWLINE;
						breakloop = 1;
					}
				}
				else if (LXRK_IsSpaceChar(lexer->kernel, c))
				{
					if (lexer->options.include_spaces)
					{
						state = LXRT_SPACE;
						breakloop = 1;
					}
				}
				else if (LXRK_IsTabChar(lexer->kernel, c))
				{
					if (lexer->options.include_tabs)
					{
						state = LXRT_TAB;
						breakloop = 1;
					}
				}
				else if (LXRK_IsWhitespaceChar(lexer->kernel, c))
				{
					// Do nothing.
				}
				else if (LXRK_IsDecimalSeparatorChar(lexer->kernel, c) && LXRK_IsDelimiterStartChar(lexer->kernel, c))
				{
					state = LXRT_STATE_POINT;
					LXR_AddToToken(lexer, c);
				}
				else if (LXRK_IsDecimalSeparatorChar(lexer->kernel, c) && !LXRK_IsDelimiterStartChar(lexer->kernel, c))
				{
					state = LXRT_STATE_FLOAT;
					LXR_AddToToken(lexer, c);
				}
				else if (LXRK_IsStringStartChar(lexer->kernel, c))
				{
					state = LXRT_STRING;
					lexer->string_end = LXRK_GetStringEnd(lexer->kernel, c);
				}
				else if (LXRK_IsDelimiterStartChar(lexer->kernel, c))
				{
					state = LXRT_DELIMITER;
					LXR_AddToToken(lexer, c);
				}
				else if (LXRK_IsUnderscoreChar(lexer->kernel, c))
				{
					state = LXRT_IDENTIFIER;
					LXR_AddToToken(lexer, c);
				}
				else if (LXRK_IsAlphabeticalChar(lexer->kernel, c))
				{
					state = LXRT_IDENTIFIER;
					LXR_AddToToken(lexer, c);
				}
				else if (c == '0')
				{
					state = LXRT_STATE_HEX_INTEGER0;
					LXR_AddToToken(lexer, c);
				}
				else if (LXRK_IsDecimalChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					LXR_AddToToken(lexer, c);
				}
				else
				{
					state = LXRT_ILLEGAL;
					LXR_AddToToken(lexer, c);
				}
			}
			break; // LXRT_UNKNOWN
			
			case LXRT_ILLEGAL:
			{
				if (c == LXRC_END_OF_STREAM) // Stream End
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsNewlineChar(lexer->kernel, c))
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsSpaceChar(lexer->kernel, c))
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsTabChar(lexer->kernel, c))
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsWhitespaceChar(lexer->kernel, c))
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsStringStartChar(lexer->kernel, c))
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsDelimiterStartChar(lexer->kernel, c))
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsAlphabeticalChar(lexer->kernel, c))
				{
					LXR_AddToToken(lexer, c);
				}
				else if (LXRK_IsDecimalChar(lexer->kernel, c))
				{
					LXR_AddToToken(lexer, c);
				}
				else
				{
					LXR_AddToToken(lexer, c);
				}
			}
			break; // LXRT_ILLEGAL
			
			case LXRT_STATE_POINT: // decimal point is seen, but it is a delimiter.
			{
				if (c == LXRC_END_OF_STREAM) // Stream End
				{
					state = LXRT_DELIMITER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsNewlineChar(lexer->kernel, c))
				{
					state = LXRT_DELIMITER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsSpaceChar(lexer->kernel, c))
				{
					state = LXRT_DELIMITER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsTabChar(lexer->kernel, c))
				{
					state = LXRT_DELIMITER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsWhitespaceChar(lexer->kernel, c))
				{
					state = LXRT_DELIMITER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsStringStartChar(lexer->kernel, c))
				{
					state = LXRT_DELIMITER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsDecimalChar(lexer->kernel, c))
				{
					state = LXRT_STATE_FLOAT;
					LXR_AddToToken(lexer, c);
				}
				else
				{
					state = LXRT_DELIMITER;
					LXR_AddToTokenTemp(lexer, c);
					if (LXRK_GetDelimiterType(lexer->kernel, lexer->token.lexeme) >= 0)
						LXR_AddToToken(lexer, c);
					else
					{
						lexer->stored = c;
						breakloop = 1;
					}
				}
			}
			break; // LXRT_STATE_POINT

			case LXRT_STATE_FLOAT:
			{
				if (c == LXRC_END_OF_STREAM) // Stream End
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsNewlineChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsSpaceChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsTabChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsWhitespaceChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (c == 'E' || c == 'e')
				{
					state = LXRT_STATE_EXPONENT;
					LXR_AddToToken(lexer, c);
				}
				else if (LXRK_IsStringStartChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsDecimalChar(lexer->kernel, c))
				{
					LXR_AddToToken(lexer, c);
				}
				else if (LXRK_IsDelimiterStartChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else
				{
					state = LXRT_ILLEGAL;
					LXR_AddToToken(lexer, c);
				}
			}
			break; // LXRT_STATE_FLOAT
			
			case LXRT_IDENTIFIER:
			{
				if (c == LXRC_END_OF_STREAM) // Stream End
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsNewlineChar(lexer->kernel, c))
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsSpaceChar(lexer->kernel, c))
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsTabChar(lexer->kernel, c))
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsWhitespaceChar(lexer->kernel, c))
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsStringStartChar(lexer->kernel, c))
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsDelimiterStartChar(lexer->kernel, c))
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsUnderscoreChar(lexer->kernel, c))
				{
					LXR_AddToToken(lexer, c);
				}
				else if (LXRK_IsAlphabeticalChar(lexer->kernel, c))
				{
					LXR_AddToToken(lexer, c);
				}
				else if (LXRK_IsDecimalChar(lexer->kernel, c))
				{
					LXR_AddToToken(lexer, c);
				}
				else
				{
					state = LXRT_ILLEGAL;
					LXR_AddToToken(lexer, c);
				}
			}
			break; // TYPE_IDENTIFIER

			case LXRT_STATE_HEX_INTEGER0:
			{
				if (c == LXRC_END_OF_STREAM) // Stream End
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsNewlineChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsSpaceChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsTabChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsWhitespaceChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsDecimalSeparatorChar(lexer->kernel, c))
				{
					state = LXRT_STATE_FLOAT;
					LXR_AddToToken(lexer, c);
				}
				else if (LXRK_IsStringStartChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsDelimiterStartChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (c == 'x' || c == 'X')
				{
					state = LXRT_STATE_HEX_INTEGER1;
					LXR_AddToToken(lexer, c);
				}
				else if (LXRK_IsDecimalChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					LXR_AddToToken(lexer, c);
				}
				else
				{
					state = LXRT_ILLEGAL;
					LXR_AddToToken(lexer, c);
				}
			}
			break; // LXRT_STATE_HEX_INTEGER0

			case LXRT_STATE_HEX_INTEGER1:
			{
				if (c == LXRC_END_OF_STREAM) // Stream End
				{
					state = LXRT_ILLEGAL;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsNewlineChar(lexer->kernel, c))
				{
					state = LXRT_ILLEGAL;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsSpaceChar(lexer->kernel, c))
				{
					state = LXRT_ILLEGAL;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsTabChar(lexer->kernel, c))
				{
					state = LXRT_ILLEGAL;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsWhitespaceChar(lexer->kernel, c))
				{
					state = LXRT_ILLEGAL;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsDecimalSeparatorChar(lexer->kernel, c))
				{
					state = LXRT_ILLEGAL;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsStringStartChar(lexer->kernel, c))
				{
					state = LXRT_ILLEGAL;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsDelimiterStartChar(lexer->kernel, c))
				{
					state = LXRT_ILLEGAL;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsHexadecimalChar(lexer->kernel, c))
				{
					state = LXRT_STATE_HEX_INTEGER;
					LXR_AddToToken(lexer, c);
				}
				else if (LXRK_IsAlphabeticalChar(lexer->kernel, c))
				{
					state = LXRT_ILLEGAL;
					LXR_AddToToken(lexer, c);
				}
				else
				{
					state = LXRT_ILLEGAL;
					LXR_AddToToken(lexer, c);
				}
			}
			break; // LXRT_STATE_HEX_INTEGER1

			case LXRT_STATE_HEX_INTEGER:
			{
				if (c == LXRC_END_OF_STREAM) // Stream End
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsNewlineChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsSpaceChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsTabChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsWhitespaceChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsStringStartChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsDelimiterStartChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsHexadecimalChar(lexer->kernel, c))
				{
					LXR_AddToToken(lexer, c);
				}
				else if (LXRK_IsAlphabeticalChar(lexer->kernel, c))
				{
					state = LXRT_ILLEGAL;
					LXR_AddToToken(lexer, c);
				}
				else
				{
					state = LXRT_ILLEGAL;
					LXR_AddToToken(lexer, c);
				}
			}
			break; // LXRT_STATE_HEX_INTEGER
			
			case LXRT_NUMBER:
			{
				if (c == LXRC_END_OF_STREAM) // Stream End
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsNewlineChar(lexer->kernel, c))
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsSpaceChar(lexer->kernel, c))
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsTabChar(lexer->kernel, c))
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsWhitespaceChar(lexer->kernel, c))
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsDecimalSeparatorChar(lexer->kernel, c))
				{
					state = LXRT_STATE_FLOAT;
					LXR_AddToToken(lexer, c);
				}
				else if (c == 'E' || c == 'e')
				{
					state = LXRT_STATE_EXPONENT;
					LXR_AddToToken(lexer, c);
				}
				else if (LXRK_IsStringStartChar(lexer->kernel, c))
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsDelimiterStartChar(lexer->kernel, c))
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsAlphabeticalChar(lexer->kernel, c))
				{
					state = LXRT_ILLEGAL;
					LXR_AddToToken(lexer, c);
				}
				else if (LXRK_IsDecimalChar(lexer->kernel, c))
				{
					LXR_AddToToken(lexer, c);
				}
				else
				{
					state = LXRT_ILLEGAL;
					LXR_AddToToken(lexer, c);
				}
			}
			break; // LXRT_NUMBER
			
			case LXRT_STATE_EXPONENT:
			{
				if (c == LXRC_END_OF_STREAM) // Stream End
				{
					state = LXRT_ILLEGAL;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsNewlineChar(lexer->kernel, c))
				{
					state = LXRT_ILLEGAL;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsSpaceChar(lexer->kernel, c))
				{
					state = LXRT_ILLEGAL;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsTabChar(lexer->kernel, c))
				{
					state = LXRT_ILLEGAL;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsWhitespaceChar(lexer->kernel, c))
				{
					state = LXRT_ILLEGAL;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsExponentSignChar(lexer->kernel, c))
				{
					state = LXRT_STATE_EXPONENT_POWER;
					LXR_AddToToken(lexer, c);
				}
				else if (LXRK_IsStringStartChar(lexer->kernel, c))
				{
					state = LXRT_ILLEGAL;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsDelimiterStartChar(lexer->kernel, c))
				{
					state = LXRT_ILLEGAL;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsAlphabeticalChar(lexer->kernel, c))
				{
					state = LXRT_ILLEGAL;
					LXR_AddToToken(lexer, c);
				}
				else if (LXRK_IsDecimalChar(lexer->kernel, c))
				{
					state = LXRT_STATE_EXPONENT_POWER;
					LXR_AddToToken(lexer, c);
				}
				else
				{
					state = LXRT_ILLEGAL;
					LXR_AddToToken(lexer, c);
				}
			}
			break; // LXRT_STATE_EXPONENT
			
			case LXRT_STATE_EXPONENT_POWER:
			{
				if (c == LXRC_END_OF_STREAM) // Stream End
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsNewlineChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsSpaceChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsTabChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsWhitespaceChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsStringStartChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsDecimalChar(lexer->kernel, c))
				{
					LXR_AddToToken(lexer, c);
				}
				else if (LXRK_IsDelimiterStartChar(lexer->kernel, c))
				{
					state = LXRT_NUMBER;
					lexer->stored = c;
					breakloop = 1;
				}
				else
				{
					state = LXRT_ILLEGAL;
					LXR_AddToToken(lexer, c);
				}
			}
			break; // LXRT_STATE_EXPONENT_POWER
			
			case LXRT_STRING:
			{
				if (c == LXRC_END_OF_STREAM) // Stream End
				{
					state = LXRT_ILLEGAL;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsNewlineChar(lexer->kernel, c))
				{
					state = LXRT_ILLEGAL;
					lexer->stored = c;
					breakloop = 1;
				}
				else if (c == lexer->string_end)
				{
					breakloop = 1;
				}
				else if (LXRK_IsEscapeChar(lexer->kernel, c))
				{
					c = LXR_GetChar(lexer);
					if (c == lexer->string_end)
						LXR_AddToToken(lexer, c);
					else if (LXRK_IsEscapeChar(lexer->kernel, c))
						LXR_AddToToken(lexer, c);
					else switch (c)
					{
						case '0':
							LXR_AddToToken(lexer, '\0');
							break;
						case 'b':
							LXR_AddToToken(lexer, '\b');
							break;
						case 't':
							LXR_AddToToken(lexer, '\t');
							break;
						case 'n':
							LXR_AddToToken(lexer, '\n');
							break;
						case 'f':
							LXR_AddToToken(lexer, '\f');
							break;
						case 'r':
							LXR_AddToToken(lexer, '\r');
							break;
						case '/':
							LXR_AddToToken(lexer, '/');
							break;

						case 'u':
						{
							h = 0;
							for (i = 0; i < 4; i++)
							{
								c = LXR_GetChar(lexer);
								if (!LXRK_IsHexadecimalChar(lexer->kernel, c))
								{
									state = LXRT_ILLEGAL;
									lexer->stored = c;
									breakloop = 1;
								}
								else
								{
									int n;
									if (c >= '0' && c <= '9')
										n = c - '0';
									else if (c >= 'A' && c <= 'F')
										n = c - 'A' + 10;
									else if (c >= 'a' && c <= 'f')
										n = c - 'a' + 10;
									h |= n << (i * 4);
								}
							}
							LXR_AddToToken(lexer, h);
						}
						break;
							
						case 'x':
						{
							h = 0;
							for (i = 0; i < 2; i++)
							{
								c = LXR_GetChar(lexer);
								if (!LXRK_IsHexadecimalChar(lexer->kernel, c))
								{
									state = LXRT_ILLEGAL;
									lexer->stored = c;
									breakloop = 1;
								}
								else
								{
									int n;
									if (c >= '0' && c <= '9')
										n = c - '0';
									else if (c >= 'A' && c <= 'F')
										n = c - 'A' + 10;
									else if (c >= 'a' && c <= 'f')
										n = c - 'a' + 10;
									h |= n << (i * 4);
								}
							}
							LXR_AddToToken(lexer, h);
						}
						break;
					}
				}
				else
				{
					LXR_AddToToken(lexer, c);
				}
			}
			break; // LXRT_STRING
			
			case LXRT_DELIMITER:
			{
				if (c == LXRC_END_OF_STREAM) // Stream End
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsNewlineChar(lexer->kernel, c))
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsSpaceChar(lexer->kernel, c))
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsTabChar(lexer->kernel, c))
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsWhitespaceChar(lexer->kernel, c))
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else if (LXRK_IsStringStartChar(lexer->kernel, c))
				{
					lexer->stored = c;
					breakloop = 1;
				}
				else
				{
					LXR_AddToTokenTemp(lexer, c);
					// Could be a special delimiter
					if ((lexer->comment_end = LXRK_GetCommentEnd(lexer->kernel, lexer->token.lexeme)) != NULL)
					{
						lexer->token.lexeme[0] = '\0';
						lexer->token.length = 0;
						state = LXRT_STATE_COMMENT;
					}
					// Could be a special delimiter
					else if (LXRK_IsLineComment(lexer->kernel, lexer->token.lexeme))
					{
						lexer->token.lexeme[0] = '\0';
						lexer->token.length = 0;
						state = LXRT_STATE_LINE_COMMENT;
					}
					// Possibly still a delimiter
					else if (LXRK_GetDelimiterType(lexer->kernel, lexer->token.lexeme) >= 0)
					{
						LXR_AddToToken(lexer, c);
					}
					// Not a delimiter anymore
					else
					{
						LXR_FlattenToken(lexer);
						lexer->stored = c;
						breakloop = 1;
					}
				}
			}
			break; // LXRT_DELIMITER

			case LXRT_STATE_COMMENT:
			{
				if (c == LXRC_END_OF_STREAM) // Stream End
				{
					lexer->token.lexeme[0] = '\0';
					lexer->token.length = 0;
					state = LXRT_UNKNOWN;
				}
				else
				{
					if (strcmp(lexer->token.lexeme, lexer->comment_end) == 0)
					{
						lexer->token.lexeme[0] = '\0';
						lexer->token.length = 0;
						state = LXRT_UNKNOWN;
					}
					else if (LXRK_IsEndCommentStartChar(lexer->kernel, c))
					{
						state = LXRT_STATE_END_COMMENT;
						LXR_AddToToken(lexer, c);
					}
				}
			}
			break; // LXRT_STATE_COMMENT
			
			case LXRT_STATE_END_COMMENT:
			{
				if (c == LXRC_END_OF_STREAM) // Stream End
				{
					lexer->token.lexeme[0] = '\0';
					lexer->token.length = 0;
					state = LXRT_STATE_COMMENT;
				}
				else if (LXRK_IsWhitespaceChar(lexer->kernel, c))
				{
					lexer->token.lexeme[0] = '\0';
					lexer->token.length = 0;
					state = LXRT_STATE_COMMENT;
				}
				else
				{
					LXR_AddToTokenTemp(lexer, c);
					if (strcmp(lexer->token.lexeme, lexer->comment_end) == 0)
					{
						lexer->token.lexeme[0] = '\0';
						lexer->token.length = 0;
						state = LXRT_UNKNOWN;
					}
					else
					{
						lexer->token.lexeme[0] = '\0';
						lexer->token.length = 0;
						LXR_AddToToken(lexer, c);
					}
				}
			}
			break; // LXRT_STATE_END_COMMENT
			
			case LXRT_STATE_LINE_COMMENT:
			{
				if (c == LXRC_END_OF_STREAM) // Stream End
				{
					lexer->token.lexeme[0] = '\0';
					lexer->token.length = 0;
					state = LXRT_UNKNOWN;
				}
				else if (LXRK_IsNewlineChar(lexer->kernel, c))
				{
					lexer->token.lexeme[0] = '\0';
					lexer->token.length = 0;
					state = LXRT_UNKNOWN;
				}
			}
			break; // LXRT_STATE_LINE_COMMENT
		
		} // switch
		
	} // while
	
	lexer_token_t *outtoken = NULL;
	
	switch (state)
	{
		case LXRT_SPACE:
		{
			lexer->token.lexeme[0] = ' ';
			lexer->token.lexeme[1] = '\0';
			lexer->token.length = 1;
		}
		break;
		
		case LXRT_TAB:
		{
			lexer->token.lexeme[0] = '\t';
			lexer->token.lexeme[1] = '\0';
			lexer->token.length = 1;
		}
		break;
		
		case LXRT_NEWLINE:
		{
			lexer->token.lexeme[0] = '\0';
			lexer->token.length = 0;
		}
		break;

		case LXRT_END_OF_LEXER:
			break;

		default:
			LXR_FinishToken(lexer, state);
			outtoken = &(lexer->token);
			break;
	}

	return outtoken;
}
