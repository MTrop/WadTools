/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "parser.h"

// ===========================================================================
// Public Functions
// ===========================================================================

// ---------------------------------------------------------------
// parser_t* PARSER_Create(lexer_t *lexer)
// See parser.h
// ---------------------------------------------------------------
parser_t* PARSER_Create(lexer_t *lexer)
{
	parser_t *out = (parser_t*)LXR_MALLOC(sizeof(parser_t));
	if (!out)
		return NULL;
	out->lexer = lexer;
	return out;
}

// ---------------------------------------------------------------
// int PARSER_Destroy(parser_t *parser)
// See parser.h
// ---------------------------------------------------------------
int PARSER_Destroy(parser_t *parser)
{
	if (!parser)
		return 1;
	
	LXR_FREE(parser);
	return 0;
}

// ---------------------------------------------------------------
// int PARSER_IsType(parser_t *parser, lexeme_type_t type)
// See parser.h
// ---------------------------------------------------------------
inline int PARSER_IsType(parser_t *parser, lexeme_type_t type)
{
	return PARSER_Current(parser)->type == type;
}

// ---------------------------------------------------------------
// int PARSER_IsSubtype(parser_t *parser, lexeme_type_t type, int subtype)
// See parser.h
// ---------------------------------------------------------------
int PARSER_IsSubtype(parser_t *parser, lexeme_type_t type, int subtype)
{
	lexer_token_t *token = PARSER_Current(parser);
	return token->type == type && token->subtype == subtype;
}

// ---------------------------------------------------------------
// int PARSER_MatchType(parser_t *parser, lexeme_type_t type)
// See parser.h
// ---------------------------------------------------------------
int PARSER_MatchType(parser_t *parser, lexeme_type_t type)
{
	if (PARSER_IsType(parser, type))
	{
		PARSER_Next(parser);
		return 1;
	}
	return 0;
}

// ---------------------------------------------------------------
// int PARSER_MatchSubtype(parser_t *parser, lexeme_type_t type, int subtype)
// See parser.h
// ---------------------------------------------------------------
int PARSER_MatchSubtype(parser_t *parser, lexeme_type_t type, int subtype)
{
	if (PARSER_IsSubtype(parser, type, subtype))
	{
		PARSER_Next(parser);
		return 1;
	}
	return 0;
}
