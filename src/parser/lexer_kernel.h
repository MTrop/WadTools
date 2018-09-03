/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#ifndef __LEXER_KERNEL_H__
#define __LEXER_KERNEL_H__

#include <stdio.h>
#include <stdint.h>
#include "lexer_config.h"
#include "../struct/mt_set.h"

#define LXR_KERNEL_SET_SIZE		32
#define LXR_KERNEL_HTABLE_SIZE	8

/**
 * Lexer kernel - defines subtypes for tokens.
 */
typedef struct {

	/** Set of pairs: char* to char*: multi-line comment-starting delimiters to comment-ending delimiters. */
	mt_set_t *comment_map;
	/** Set of pairs: char* to int: line-comment delimiters to comment type. */
	mt_set_t *comment_line_map;

	/** Set of pairs: char* to int: delimiters to delimiter type. */
	mt_set_t *delimiter_map;
	
	/** Set of characters that start delimiters. Delimiters immediately break the current token if encountered. */
	mt_set_t *delimiter_starts;
	
	/** Set of pairs: char* to int: identifier to keyword type. */
	mt_set_t *keyword_map;
	/** Set of pairs: char* to int: identifier to keyword type. Case-insensitive matching. */
	mt_set_t *cikeyword_map;

	/** Set of pairs: char to char: string start char to string end char. */
	mt_set_t *string_map;

	/** Decimal character. */
	char decimal_char;
	/** String escape character. */
	char escape_char;

} lexer_kernel_t;

//............................................................................

/**
 * Creates a new lexer kernel.
 * @return a new lexer kernel or NULL if it couldn't be allocated.
 */
lexer_kernel_t* LXRK_Create();

/**
 * Destroys an allocated lexer kernel.
 * @param kernel the kernel to destroy.
 * @return 0 is successful and the pointer was invalidated, nonzero if not.
 */
int LXRK_Destroy(lexer_kernel_t *kernel);

//............................................................................

/**
 * Adds a multi-line comment delimiter type.
 * @param kernel the kernel to add to.
 * @param comment_start the starting token for the comment.
 * @param comment_end the ending token for the comment.
 */
void LXRK_AddCommentDelimiter(lexer_kernel_t *kernel, char *comment_start, char *comment_end);

/**
 * Adds a multi-line comment delimiter type.
 * @param kernel the kernel to add to.
 * @param delimiter the starting token for the line comment.
 */
void LXRK_AddLineCommentDelimiter(lexer_kernel_t *kernel, char *delimiter);

/**
 * Adds a delimiter type.
 * @param kernel the kernel to add to.
 * @param delimiter the string for the delimiter.
 * @param delimiter_type the token subtype.
 */
void LXRK_AddDelimiter(lexer_kernel_t *kernel, char *delimiter, int delimiter_type);

/**
 * Adds a reserved keyword type.
 * Must be a subset of an identifier type.
 * @param kernel the kernel to add to.
 * @param keyword the starting character.
 * @param keyword_type the associated subtype.
 */
void LXRK_AddKeyword(lexer_kernel_t *kernel, char *keyword, int keyword_type);

/**
 * Adds a case-insensitive reserved keyword type.
 * Must be a subset of an identifier type.
 * @param kernel the kernel to add to.
 * @param keyword the starting character.
 * @param keyword_type the associated subtype.
 */
void LXRK_AddCaseInsensitiveKeyword(lexer_kernel_t *kernel, char *keyword, int keyword_type);

/**
 * Adds a string delimiter type.
 * @param kernel the kernel to add to.
 * @param start the starting character.
 * @param end the ending character.
 */
void LXRK_AddStringDelimiters(lexer_kernel_t *kernel, char start, char end);

/**
 * Sets the decimal separator character. Default is '.'
 * @param kernel the kernel to add to.
 * @param separator the character to use.
 */
void LXRK_SetDecimalSeparator(lexer_kernel_t *kernel, char separator);

/**
 * Sets the in-string escape character. Default is '\'.
 * @param kernel the kernel to add to.
 * @param escape the character to use.
 */
void LXRK_SetStringEscapeChar(lexer_kernel_t *kernel, char escape);

//............................................................................

/**
 * Gets the associated comment end string.
 * @param kernel the kernel to add to.
 * @param comment_start the starting token for the comment.
 * @return the comment end or NULL for no associated end.
 */
char* LXRK_GetCommentEnd(lexer_kernel_t *kernel, char *comment_start);

//............................................................................

/**
 * Checks if a character is a alphabetical (letter) character.
 * @param kernel the kernel to use.
 * @param c the character to test.
 * @return 1 if so, 0 if not.
 */
int LXRK_IsAlphabeticalChar(lexer_kernel_t *kernel, int c);

/**
 * Checks if a character is a hexadecimal character.
 * @param kernel the kernel to use.
 * @param c the character to test.
 * @return 1 if so, 0 if not.
 */
int LXRK_IsHexadecimalChar(lexer_kernel_t *kernel, int c);

/**
 * Checks if a character is a decimal character.
 * @param kernel the kernel to use.
 * @param c the character to test.
 * @return 1 if so, 0 if not.
 */
int LXRK_IsDecimalChar(lexer_kernel_t *kernel, int c);

/**
 * Checks if a character is an underscore character.
 * @param kernel the kernel to use.
 * @param c the character to test.
 * @return 1 if so, 0 if not.
 */
int LXRK_IsUnderscoreChar(lexer_kernel_t *kernel, int c);

/**
 * Checks if a character is a newline character.
 * @param kernel the kernel to use.
 * @param c the character to test.
 * @return 1 if so, 0 if not.
 */
int LXRK_IsNewlineChar(lexer_kernel_t *kernel, int c);

/**
 * Checks if a character is a space character.
 * @param kernel the kernel to use.
 * @param c the character to test.
 * @return 1 if so, 0 if not.
 */
int LXRK_IsSpaceChar(lexer_kernel_t *kernel, int c);

/**
 * Checks if a character is a tab character.
 * @param kernel the kernel to use.
 * @param c the character to test.
 * @return 1 if so, 0 if not.
 */
int LXRK_IsTabChar(lexer_kernel_t *kernel, int c);

/**
 * Checks if a character is a whitespace character.
 * @param kernel the kernel to use.
 * @param c the character to test.
 * @return 1 if so, 0 if not.
 */
int LXRK_IsWhitespaceChar(lexer_kernel_t *kernel, int c);

/**
 * Checks if a character is an exponent sign character.
 * @param kernel the kernel to use.
 * @param c the character to test.
 * @return 1 if so, 0 if not.
 */
int LXRK_IsExponentSignChar(lexer_kernel_t *kernel, int c);

/**
 * Checks if a character is a delimiter character.
 * @param kernel the kernel to use.
 * @param c the character to test.
 * @return 1 if so, 0 if not.
 */
int LXRK_IsDelimiterChar(lexer_kernel_t *kernel, int c);

/**
 * Checks if a character is a string-start character.
 * @param kernel the kernel to use.
 * @param c the character to test.
 * @return 1 if so, 0 if not.
 */
int LXRK_IsStringStartChar(lexer_kernel_t *kernel, int c);

/**
 * Checks if a character is the decimal separator character.
 * @param kernel the kernel to use.
 * @param c the character to test.
 * @return 1 if so, 0 if not.
 */
int LXRK_IsDecimalSeparatorChar(lexer_kernel_t *kernel, int c);

/**
 * Checks if a character is the in-string escape character.
 * @param kernel the kernel to use.
 * @param c the character to test.
 * @return 1 if so, 0 if not.
 */
int LXRK_IsEscapeChar(lexer_kernel_t *kernel, int c);

#endif
