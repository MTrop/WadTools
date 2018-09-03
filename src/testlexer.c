#include <stdio.h>
#include <errno.h>
#include "parser/lexer_kernel.h"

int main(int argc, char** argv)
{
	lexer_kernel_t *kernel = LXRK_Create();
	LXRK_AddCommentDelimiter(kernel, "/*", "*/");
	LXRK_AddLineCommentDelimiter(kernel, "//");
	LXRK_Destroy(kernel);
	return 0;
}
