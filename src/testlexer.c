#include <stdio.h>
#include <errno.h>
#include "parser/lexer.h"

void dumpToken(lexer_token_t *token)
{
	printf(
		"(%s) Line %-4d Len %-3d %s:%d, %s\n", 
		token->stream->name, 
		token->line_number, 
		token->length, 
		LXR_TokenTypeName(token->type), 
		token->subtype, 
		token->lexeme
	);
}

int main(int argc, char** argv)
{
	lexer_kernel_t *kernel = LXRK_Create();
	//LXRK_AddCommentDelimiter(kernel, "/*", "*/");
	//LXRK_AddLineCommentDelimiter(kernel, "//");
	LXRK_AddStringDelimiters(kernel, '"', '"');
	
	lexer_t *lexer = LXR_Create(kernel);
	LXR_PushStream(lexer, "src/test.c");
	
	lexer_token_t *token;
	while ((token = LXR_NextToken(lexer)) != NULL)
		dumpToken(token);

	LXR_Destroy(lexer);
	LXRK_Destroy(kernel);
	return 0;
}
