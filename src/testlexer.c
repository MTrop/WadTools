#include <stdio.h>
#include <errno.h>
#include "parser/lexer.h"

void dumpToken(lexer_token_t *token)
{
	printf(
		"(%s) Line %-4d Len %-3d %-22s:%-4d  %s\n", 
		token->stream->name, 
		token->line_number, 
		token->length, 
		LXR_TokenTypeName(token->type), 
		token->subtype, 
		token->lexeme
	);
}

void doParse(lexer_kernel_t *kernel, char* filename)
{
	lexer_t *lexer = LXR_Create(kernel);
	LXR_PushStream(lexer, filename);
	
	lexer_token_t *token;
	while ((token = LXR_NextToken(lexer)) != NULL)
		dumpToken(token);

	LXR_Destroy(lexer);
}

typedef struct {
	
	char *key;
	int value;
	
} PAIR;


void delimDumpFunc(void *p)
{
	PAIR *l = p;
	printf("[\"%s\", %d]\n", l->key, l->value);
}

int main(int argc, char** argv)
{
	lexer_kernel_t *kernel = LXRK_Create();
	LXRK_AddCommentDelimiter(kernel, "/*", "*/");
	LXRK_AddLineCommentDelimiter(kernel, "//");
	LXRK_AddStringDelimiters(kernel, '"', '"');
	LXRK_AddStringDelimiters(kernel, '\'', '\'');

	LXRK_AddKeyword(kernel, "return", 1);
	LXRK_AddKeyword(kernel, "int", 2);

	LXRK_AddDelimiter(kernel, "#", 1);
	LXRK_AddDelimiter(kernel, "<", 2);
	LXRK_AddDelimiter(kernel, ">", 3);
	LXRK_AddDelimiter(kernel, "+", 4);
	LXRK_AddDelimiter(kernel, "-", 5);
	LXRK_AddDelimiter(kernel, "*", 6);
	LXRK_AddDelimiter(kernel, "/", 7);
	LXRK_AddDelimiter(kernel, ")", 8);
	LXRK_AddDelimiter(kernel, "(", 9);
	LXRK_AddDelimiter(kernel, "[", 10);
	LXRK_AddDelimiter(kernel, "]", 11);
	LXRK_AddDelimiter(kernel, "{", 12);
	LXRK_AddDelimiter(kernel, "}", 13);
	LXRK_AddDelimiter(kernel, ".", 14);
	LXRK_AddDelimiter(kernel, ",", 15);
	LXRK_AddDelimiter(kernel, "%", 16);
	LXRK_AddDelimiter(kernel, "!", 17);
	LXRK_AddDelimiter(kernel, "?", 18);
	LXRK_AddDelimiter(kernel, "=", 19);
	LXRK_AddDelimiter(kernel, "==", 20);
	LXRK_AddDelimiter(kernel, "!=", 21);
	LXRK_AddDelimiter(kernel, "++", 22);
	LXRK_AddDelimiter(kernel, "--", 23);
	LXRK_AddDelimiter(kernel, "+=", 24);
	LXRK_AddDelimiter(kernel, "-=", 25);
	LXRK_AddDelimiter(kernel, "*=", 26);
	LXRK_AddDelimiter(kernel, "/=", 27);
	LXRK_AddDelimiter(kernel, ";", 28);
	LXRK_AddDelimiter(kernel, ":", 29);
	LXRK_AddDelimiter(kernel, "%=", 30);

	doParse(kernel, "src/parsertest.txt");

	LXRK_Destroy(kernel);
	return 0;
}
