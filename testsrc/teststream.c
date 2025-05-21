#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "io/stream.h"

char buffer[1024];

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("ERROR: need file\n");
		return 1;
	}

	//stream_t* stream = STREAM_Open(argv[1]);
	//stream_t* stream = STREAM_OpenBuffered(argv[1], 1024);
	//FILE *fp = fopen(argv[1], "rb");
	//fseek(fp, 181, SEEK_SET);
	//stream_t* stream = STREAM_OpenFile(fp);
	//stream_t* stream = STREAM_OpenBufferedFile(fp, 1024);
	//stream_t* stream = STREAM_OpenFileSection(fp, 87);
	//stream_t* stream = STREAM_OpenBufferedFileSection(fp, 87, 1024);
	const char *str = "abcdefghijklmnopqrstuvwxyz0123456789";
	stream_t* stream = STREAM_OpenBuffer((unsigned char*)str, strlen(str));
	
	if (!stream)
	{
		printf("ERROR: %s\n", strerror(errno));
		return 1;
	}

	puts("=================");
	STREAM_Dump(stream);
	puts("=================");
	
	int c;
	while ((c = STREAM_GetChar(stream)) != EOF)
		printf("%c\n", c);
	while (STREAM_ReadLine(stream, buffer, 1024) != EOF)
		printf("%s\n", buffer);

	STREAM_Reset(stream);
	puts("=================");
	STREAM_Dump(stream);
	puts("=================");

	while (STREAM_ReadLine(stream, buffer, 1024) != EOF)
		printf("%s\n", buffer);
	
	puts("=================");
	STREAM_Dump(stream);
	puts("=================");

	STREAM_Close(stream);
	//fclose(fp);
	return 0;
}
