/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#ifndef __LEXER_H__
#define __LEXER_H__

#include <stdio.h>
#include "lexer_config.h"
#include "lexer_kernel.h"
#include "io/stream.h"

/**
 * Lexeme type.
 */
typedef enum {
	
	/** Token type: End of lexer. */
	LXRT_END_OF_LEXER,
	/** Token type: End of stream. */
	LXRT_END_OF_STREAM,
	/** Token type: Unknown token. */
	LXRT_UNKNOWN,
	/** Token type: Illegal token. */
	LXRT_ILLEGAL,
	/** Token type: Space. */
	LXRT_SPACE,
	/** Token type: Tab. */
	LXRT_TAB,
	/** Token type: New line character. */
	LXRT_NEWLINE,
	/** Token type: Identifier. */
	LXRT_IDENTIFIER,
	/** Token type: Keyword. */
	LXRT_KEYWORD,
	/** Token type: Delimiter. */
	LXRT_DELIMITER,
	/** Token type: Number. */
	LXRT_NUMBER,
	/** Token type: String. */
	LXRT_STRING,
	/** Token type: Point state (never returned). */
	LXRT_STATE_POINT,
	/** Token type: Floating point state (never returned). */
	LXRT_STATE_FLOAT,
	/** Token type: Delimiter Comment (never returned). */
	LXRT_STATE_COMMENT,
	/** Token type: Delimiter Comment (never returned). */
	LXRT_STATE_END_COMMENT,
	/** Token type: Delimiter Comment (never returned). */
	LXRT_STATE_LINE_COMMENT,
	/** Token type: hexadecimal integer (never returned). */
	LXRT_STATE_HEX_INTEGER0,
	/** Token type: hexadecimal integer (never returned). */
	LXRT_STATE_HEX_INTEGER1,
	/** Token type: hexadecimal integer (never returned). */
	LXRT_STATE_HEX_INTEGER,
	/** Token type: Exponent state (never returned). */
	LXRT_STATE_EXPONENT,
	/** Token type: Exponent power state (never returned). */
	LXRT_STATE_EXPONENT_POWER,
	
	LXRT_COUNT,
	
} lexeme_type_t;

/** Lexer Numeric token subtype. */
typedef enum {

	/** Integer */
	LXRTN_INTEGER,
	/** Floating point */
	LXRTN_FLOAT,
	/** Octal Integer */
	LXRTN_OCTAL,
	/** Hex Integer */
	LXRTN_HEX,

	LXRTN_COUNT,

} lexeme_numeric_subtype_t;

/**
 * Lexer options.
 */
typedef struct {

	/** Include spaces? */
	int include_spaces;
	/** Include tabs? */
	int include_tabs;
	/** Include newlines? */
	int include_newlines;
	/** Include stream break (end of stream)? */
	int include_stream_break;

} lexer_options_t;

/**
 * An input stream for the lexer.
 */
typedef struct {

	/** Stream name. */
	char *name;
	/** Source stream. */
	stream_t *stream;
	/** Current line number. */
	int line_number;
	/** Current character number (on the line). */
	int character_number;

} lexer_stream_t;

/**
 * A scanned lexer token.
 */
typedef struct {
	
	/** Source stream name. */
	char *stream_name;
	/** Lexeme type. */
	lexeme_type_t type;
	/** Token subtype. */
	int subtype;
	/** Token lexeme. */
	char lexeme[LEXEME_LENGTH_MAX];
	/** Token lexeme length. */
	int length;
	/** Current line number. */
	int line_number;
	
} lexer_token_t;

/**
 * Single stack node.
 */
typedef struct LEXERSS_NODE {
	
	/** Lexer stream. */
	lexer_stream_t *stream;
	/** Previous node in the stack. */
	struct LEXERSS_NODE *previous;
	
} lexer_stream_stack_t;

/**
 * Lexer.
 */
typedef struct {

	/** Kernel. */
	lexer_kernel_t *kernel;
	/** Currently scanned token. */
	lexer_token_t token;
	/** Lexer options. */
	lexer_options_t options;

	/** Stream stack. */
	lexer_stream_stack_t *stream_stack;
	/** Current lexer state. */
	lexeme_type_t state;
	/** Current string terminal. */
	char string_end;
	/** Current comment terminal. */
	char *comment_end;
	/** Stored character on token break. */
	char stored;

} lexer_t;

/**
 * Creates a new lexer.
 * @param kernel the kernel to use. Cannot be NULL.
 * @return a new lexer or NULL if it couldn't be allocated.
 */
lexer_t* LXR_Create(lexer_kernel_t *kernel);

/**
 * Closes a lexer.
 * All pushed streams that are still open are closed and freed.
 * If successful, the pointer is invalidated.
 * The underlying kernel is NOT FREED.
 * @param lexer the lexer to close.
 * @return 0 if successful and the pointer was invalidated, nonzero if not.
 */
int LXR_Destroy(lexer_t *lexer);

/**
 * Pushes a new character stream onto the lexer using a file.
 * The name of the stream is the file name.
 * NOTE: Be careful - this does not affect the current state!
 * @param lexer the lexer to use.
 * @param filename the filename to open.
 * @return 0 if successful or nonzero if not.
 */
int LXR_PushStream(lexer_t *lexer, char *filename);

/**
 * Pushes a new character stream onto the lexer using an already-opened file.
 * NOTE: Be careful - this does not affect the current state!
 * @param lexer the lexer to use.
 * @param name the name of the stream.
 * @param file the open file to wrap as a stream.
 * @return 0 if successful or nonzero if not.
 */
int LXR_PushStreamFile(lexer_t *lexer, char *name, FILE *file);

/**
 * Pushes a new character stream onto the lexer using a byte buffer.
 * NOTE: Be careful - this does not affect the current state!
 * @param lexer the lexer to use.
 * @param name the name of the stream.
 * @param buffer the buffer to wrap as a stream.
 * @param length the length of the buffer in bytes.
 * @return 0 if successful or nonzero if not.
 */
int LXR_PushStreamBuffer(lexer_t *lexer, char *name, unsigned char *buffer, size_t length);

/**
 * Pops a character stream off of lexer.
 * NOTE: Be careful - this does not affect the current state!
 * @param lexer the lexer to use.
 * @return 0 if successful or nonzero if not.
 */
int LXR_PopStream(lexer_t *lexer);

/**
 * Scans the next lexeme and returns the token.
 * NOTE: The lexeme scanned should be copied if the content itself needs to be used elsewhere.
 * @param lexer the lexer to use.
 * @return a pointer to the token scanned in.
 */
lexer_token_t* LXR_NextToken(lexer_t *lexer);

/**
 * Returns the type name for a token type.
 * @param type the lexeme type.
 * @return a string or NULL if no corresponding name.
 */
char* LXR_TokenTypeName(lexeme_type_t type);

/**
 * Returns the subtype name for a numberic token subtype.
 * @param subtype the subtype.
 * @return a string or NULL if no corresponding name.
 */
char* LXR_TokenNumericSubtypeName(lexeme_numeric_subtype_t subtype);

#endif