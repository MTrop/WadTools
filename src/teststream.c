#include <stdio.h>
#include <errno.h>
#include "io/stream.h"

char buffer[1024];

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("ERROR: need file\n");
		return 1;
	}

	stream_t* stream = STREAM_Open(argv[1]);
	// TODO: Not working.
	//stream_t* stream = STREAM_OpenBuffered(argv[1], 1024);
	
	if (!stream)
	{
		printf("ERROR: %s\n", strerror(errno));
		return 1;
	}
	
	while (STREAM_ReadLine(stream, buffer, 1024) != EOF)
		printf("%s\n", buffer);
	
	STREAM_Close(stream);
	return 0;
}
