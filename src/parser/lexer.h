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
#include "../mt_vector.h"
#include "../mt_set.h"
#include "../mt_htable.h"

/**
 * Lexeme type.
 */
typedef enum {
	
	/** Reserved token type: End of lexer. */
	LXT_END_OF_LEXER,
	/** Reserved token type: End of stream. */
	LXT_END_OF_STREAM,
	/** Reserved token type: Number. */
	LXT_NUMBER,
	/** Reserved token type: Space. */
	LXT_DELIM_SPACE,
	/** Reserved token type: Tab. */
	LXT_DELIM_TAB,
	/** Reserved token type: New line character. */
	LXT_DELIM_NEWLINE,
	/** Reserved token type: Open comment. */
	LXT_DELIM_OPEN_COMMENT,
	/** Reserved token type: Close comment. */
	LXT_DELIM_CLOSE_COMMENT,
	/** Reserved token type: Line comment. */
	LXT_DELIM_LINE_COMMENT,
	/** Reserved token type: Identifier. */
	LXT_IDENTIFIER,
	/** Reserved token type: Unknown token. */
	LXT_UNKNOWN,
	/** Reserved token type: Illegal token. */
	LXT_ILLEGAL,
	/** Reserved token type: Comment. */
	LXT_COMMENT,
	/** Reserved token type: Line Comment. */
	LXT_LINE_COMMENT,
	/** Reserved token type: String. */
	LXT_STRING,
	/** Reserved token type: Special (never returned). */
	LXT_SPECIAL,
	/** Reserved token type: Delimiter (never returned). */
	LXT_DELIMITER,
	/** Reserved token type: Point state (never returned). */
	LXT_POINT,
	/** Reserved token type: Floating point state (never returned). */
	LXT_FLOAT,
	/** Reserved token type: Delimiter Comment (never returned). */
	LXT_DELIM_COMMENT,
	/** Reserved token type: hexadecimal integer (never returned). */
	LXT_HEX_INTEGER0,
	/** Reserved token type: hexadecimal integer (never returned). */
	LXT_HEX_INTEGER1,
	/** Reserved token type: hexadecimal integer (never returned). */
	LXT_HEX_INTEGER,
	/** Reserved token type: Exponent state (never returned). */
	LXT_EXPONENT,
	/** Reserved token type: Exponent power state (never returned). */
	LXT_EXPONENT_POWER;
	
} lexeme_type_t;


/**
 * Lexer options.
 */
typedef struct {

	/** Include spaces? */
	int include_spaces;
	/** Include tabs? */
	int include_spaces;
	/** Include newlines? */
	int include_newlines;
	/** Include stream break? */
	int include_stream_break;
	/** Decimal character. */
	int decimal_char;

} lexer_options_t;

/**
 * An input stream for the lexer.
 */
typedef struct {

	/** Stream name. */
	char *name;

	union {
		FILE *stream;
	} so
	
	/** Current line number. */
	int linenum;
	/** Current character number (on the line). */
	int charnum;

} lexer_stream_t;


/**
 * A scanned lexer token.
 */
typedef struct {
	
	/** Lexeme type. */
	lexeme_type_t *type;
	/** Token keyword type. */
	int keyword_type;
	/** Token lexeme. */
	char *lexeme;
	
} lexer_token_t;


/**
 * Lexer.
 */
typedef struct {

	// TODO: Finish this.
	
} lexer_t;

/**
 * Creates a new stream that a lexer can read from.
 * The stream name is the file's name.
 * @param filename the name of the file to open.
 * @return a new lexer stream or NULL if it couldn't be opened or allocated.
 */
lexer_stream_t* lexer_stream_open_file(char *filename);

/**
 * Creates a new stream that a lexer can read from.
 * @param name the stream name.
 * @param file the open stream.
 * @return a new lexer stream or NULL if it couldn't be opened or allocated.
 */
lexer_stream_t* lexer_stream_create_file(char *name, FILE *stream);

/**
 * Creates a new stream that a lexer can read from.
 * @param name the stream name.
 * @param charstream the stream of characters.
 * @param length the amount of characters.
 * @return a new lexer stream or NULL if it couldn't be opened or allocated.
 */
lexer_stream_t* lexer_stream_create_buffer(char *name, char *charstream, int length);

/**
 * Closes a lexer stream.
 * If successful, the pointer is invalidated.
 * @param stream the stream.
 * @return 0 if successful and the pointer was invalidated, nonzero if not.
 */
int lexer_stream_close(lexer_stream_t *stream);


#endif