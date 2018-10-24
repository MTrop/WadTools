/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#ifndef __PARSER_H__
#define __PARSER_H__

#include <stdio.h>
#include "lexer.h"

/**
 * A parser that encloses one lexer.
 */
typedef struct {

	/** Current lexer. */
	lexer_t *lexer;

} parser_t;

/**
 * Creates a new parser.
 * PARSER_Next() must be called or else this will not 
 * @param lexer the lexer to encapsulate. Cannot be NULL.
 * @return a new lexer or NULL if it couldn't be allocated.
 */
parser_t* PARSER_Create(lexer_t *lexer);

/**
 * Closes a parser.
 * If successful, the pointer is invalidated.
 * The underlying lexer is NOT FREED NOR ALTERED.
 * @param parser the parser to destroy.
 * @return 0 if successful and the pointer was invalidated, nonzero if not.
 */
int PARSER_Destroy(parser_t *parser);

/**
 * Gets the next token from the underlying lexer.
 * @param p the parser to use.
 * @return the pointer to the next token.
 * @see lexer.h/LXR_NextToken(lexer_token_t*)
 */
#define PARSER_Next(p) LXR_NextToken((p)->lexer)

/**
 * Gets the current token from the underlying lexer.
 * @param p the parser to use.
 * @return the pointer to the current token.
 * @see lexer.h/LXR_NextToken(lexer_token_t*)
 */
#define PARSER_Current(p) (&((p)->lexer->token))

/**
 * Checks if the current lexeme type on the current token is the provided type.
 * @param type the type to check.
 * @return 1 if so, 0 if not.
 */
int PARSER_IsType(parser_t *parser, lexeme_type_t type);

/**
 * Checks if the current lexeme type and subtype on the current token are the provided types.
 * @param type the type to check.
 * @param subtype the subtype to check.
 * @return 1 if so, 0 if not.
 */
int PARSER_IsSubtype(parser_t *parser, lexeme_type_t type, int subtype);

/**
 * Attempts to match the current lexeme type on the current token.
 * If the type matches, the next token is automatically fetched.
 * @param type the type to check.
 * @return 1 if so, 0 if not.
 */
int PARSER_MatchType(parser_t *parser, lexeme_type_t type);

/**
 * Attempts to match the current lexeme type and subtype on the current token are the provided types.
 * If the types match, the next token is automatically fetched.
 * @param type the type to check.
 * @param subtype the subtype to check.
 * @return 1 if so, 0 if not.
 */
int PARSER_MatchSubtype(parser_t *parser, lexeme_type_t type, int subtype);

#endif