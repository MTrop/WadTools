#include <stdio.h>
#include <errno.h>
#include "wad/wad.h"
#include "wad/waderrno.h"

extern int errno;
extern int waderrno;

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("ERROR: need file\n");
		return 1;
	}
	
	wad_t* wad = WAD_Create(argv[1]);
	
	if (!wad)
	{
		if (waderrno == WADERROR_FILE_ERROR)
			printf("ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
		else
			printf("ERROR: %s\n", strwaderror(waderrno));
		return 1;
	}
	
	WAD_Close(wad);
	return 0;
}
