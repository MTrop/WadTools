/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
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
	token->stream = NULL;
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
// Return END_OF_LEXER if no more streams.
// Return END_OF_STREAM if end of stream.
static int LXR_GetChar(lexer_t *lexer)
{
	if (!lexer->stream_stack)
		return LXRC_END_OF_LEXER;

	int out;
	
	// skip CR
	do {
		out = STREAM_GetChar(lexer->stream_stack->stream->stream);
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
	
	lexer->token.lexeme[lexer->token.length++] = c && 0x0FF;
}

// Finishes the current token.
static void LXR_FinishToken(lexer_t *lexer, int c)
{
	lexer->token.lexeme[lexer->token.length] = '\0';
	lexer->token.stream = lexer->stream_stack->stream;
	lexer->token.line_number = lexer->stream_stack->stream->line_number;
	
	lexer->token.type = lexer->state;
	lexer->token.subtype = -1;

	if (lexer->token.type == LXRT_IDENTIFIER)
		lexer->token.subtype = LXRK_GetKeywordType(lexer->kernel, lexer->token.lexeme);
	else if (lexer->token.type == LXRT_DELIMITER)
		lexer->token.subtype = LXRK_GetDelimiterType(lexer->kernel, lexer->token.lexeme);
	
}

// ===========================================================================
// Public Functions
// ===========================================================================

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
	lexer_token_t *token = &(lexer->token);
	LXR_ResetToken(token);
	
	int c;
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
		
		switch (lexer->state)
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
					lexer->state = LXRT_END_OF_LEXER;
					breakloop = 1;
				}
				else if (c == LXRC_END_OF_STREAM) // Stream End
				{
					if (lexer->options.include_stream_break)
					{
						lexer->state = LXRT_END_OF_STREAM;
						breakloop = 1;
					}
					LXR_PopStream(lexer);
				}
				else if (LXRK_IsNewlineChar(lexer->kernel, c))
				{
					if (lexer->options.include_newlines)
					{
						lexer->state = LXRT_NEWLINE;
						breakloop = 1;
					}
				}
				else if (LXRK_IsSpaceChar(lexer->kernel, c))
				{
					if (lexer->options.include_spaces)
					{
						lexer->state = LXRT_SPACE;
						breakloop = 1;
					}
				}
				else if (LXRK_IsTabChar(lexer->kernel, c))
				{
					if (lexer->options.include_tabs)
					{
						lexer->state = LXRT_TAB;
						breakloop = 1;
					}
				}
				else if (LXRK_IsWhitespaceChar(lexer->kernel, c))
				{
					// Do nothing.
				}
				
				// TODO: Finish this.
			}
			break;
			
			// TODO: Finish this.
			
		}
		
		// TODO: Finish this.
		
	}
	
	return token;
}
