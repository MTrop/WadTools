/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "io/stream.h"
#include "wadio/wadstream.h"
#include "wadtool.h"
#include "wad/wad.h"
#include "wad/waderrno.h"
#include "wad/wad_config.h"

extern int errno;
extern int waderrno;

#define ERRORCLEAN_NONE          0
#define ERRORCLEAN_NO_FILENAME   1
#define ERRORCLEAN_OUT_OF_MEMORY 2
#define ERRORCLEAN_WAD_ERROR     10
#define ERRORCLEAN_IO_ERROR      20

typedef struct
{
	/** WAD filename. */
	char *filename;
	/** The WAD to use. */
	wad_t *wad;

	/** Destination path. */
	char *outpath;
	/** Same output flag. */
	int same_output;
	/** If nonzero, verbose output. */
	int verbose;

} wadtool_options_clean_t;

typedef struct
{
	/** Buffer length in bytes. */
	size_t len;
	/** The buffer itself. */
	void *buffer;

} databuffer_t;

static int destroy_buffer(databuffer_t *buf)
{
	WAD_FREE(buf->buffer);
	return 0;
}

static int alloc_buffer(databuffer_t *buf)
{
	buf->buffer = WAD_MALLOC(buf->len);
	return buf->buffer != NULL;
}

static int realloc_buffer(databuffer_t *buf, size_t newsize)
{
	destroy_buffer(buf);
	buf->len = newsize;
	return alloc_buffer(buf);
}

static int print_waderrno()
{
	if (!waderrno)
		return ERRORCLEAN_NONE;

	if (waderrno == WADERROR_FILE_ERROR)
	{
		fprintf(stderr, "ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
		return ERRORCLEAN_IO_ERROR + errno;
	}
	else
	{
		fprintf(stderr, "ERROR: %s\n", strwaderror(waderrno));
		return ERRORCLEAN_WAD_ERROR + waderrno;
	}
}

static int exec(wadtool_options_clean_t *options)
{
	char *outwadpath = options->outpath;

	// if same path for output, then append a new extension to temp file
	if (options->same_output)
	{
		outwadpath = (char*)WAD_MALLOC(strlen(options->outpath) + 5); // ext plus null char
		sprintf(outwadpath, "%s.new", options->outpath);
	}

	wad_t *srcwad = options->wad;
	wad_t *destwad = WAD_Create(outwadpath);

	if (!destwad)
	{
		if (options->same_output)
			WAD_FREE(outwadpath);
		return print_waderrno();
	}

	waditerator_t srciter;
	WAD_IteratorInit(&srciter, srcwad, 0);

	// iterate through source WAD, build output WAD from it.

	// Create buffer for WAD data.
	databuffer_t dbuf;
	dbuf.len = 8192; // 8kb, to start
	alloc_buffer(&dbuf);

	// need to pad src entry names with null char
	char srcentryname[9];

	wadentry_t *srcentry;
	while ((srcentry = WAD_IteratorNext(&srciter)))
	{
		// get null-terminated name
		sprintf(srcentryname, "%-.8s", srcentry->name);

		// if not marker....
		if (srcentry->length > 0) 
		{
			// Allocate enough bytes for incoming data.
			if (srcentry->length > dbuf.len)
			{
				if (!realloc_buffer(&dbuf, srcentry->length))
				{
					if (options->same_output)
						WAD_FREE(outwadpath);
					destroy_buffer(&dbuf);
					fprintf(stderr, "ERROR: Not enough memory for transfer operation: %d\n", srcentry->length);
					return ERRORCLEAN_OUT_OF_MEMORY;
				}
			}

			// Fetch the data.
			int datalen = WAD_GetEntryData(srcwad, srcentry, dbuf.buffer);
			if (datalen < 0)
			{
				if (options->same_output)
					WAD_FREE(outwadpath);
				destroy_buffer(&dbuf);
				return print_waderrno();
			}

			wadentry_t *destentry = WAD_AddEntry(destwad, srcentryname, dbuf.buffer, datalen);
			if (!destentry)
			{
				if (options->same_output)
					WAD_FREE(outwadpath);
				destroy_buffer(&dbuf);
				return print_waderrno();
			}

			if (options->verbose)
				printf("Moved entry %s (%d bytes)...\n", srcentryname, datalen);
		}
		else // marker
		{
			if (!WAD_AddMarkerEntry(destwad, srcentryname))
			{
				if (options->same_output)
					WAD_FREE(outwadpath);
				destroy_buffer(&dbuf);
				return print_waderrno();
			}

			if (options->verbose)
				printf("Added marker entry %s...\n", srcentryname);
		}

	} // end-while

	destroy_buffer(&dbuf);
	WAD_Close(destwad);

	if (options->same_output)
	{
		// do a switcheroo
		WAD_Close(options->wad);
		if (remove(options->filename))
		{
			fprintf(stderr, "ERROR: Could not remove file: %s\n", options->filename);
			WAD_FREE(outwadpath);
			return ERRORCLEAN_IO_ERROR;
		}
		if (rename(outwadpath, options->filename))
		{
			fprintf(stderr, "ERROR: Could not rename temporary file: %s\n", outwadpath);
			WAD_FREE(outwadpath);
			return ERRORCLEAN_IO_ERROR;
		}

		// will be closed when exec() completes
		options->wad = WAD_Open(options->filename);

		WAD_FREE(outwadpath);
		printf("Cleaned %s.\n", options->filename);
	}
	else
	{
		printf("Cleaned %s as %s.\n", options->filename, options->outpath);
	}

	return ERRORCLEAN_NONE;
}

// If nonzero, bad parse.
static int parse_file(arg_parser_t *argparser, wadtool_options_clean_t *options)
{
	options->filename = currarg(argparser);
	if (!options->filename)
	{
		fprintf(stderr, "ERROR: No WAD file.\n");
		return ERRORCLEAN_NO_FILENAME;
	}

	// Open a file.
	options->wad = WAD_Open(options->filename);

	if (!options->wad)
	{
		if (waderrno == WADERROR_FILE_ERROR)
		{
			fprintf(stderr, "ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
			return ERRORCLEAN_IO_ERROR + errno;
		}
		else
		{
			fprintf(stderr, "ERROR: %s\n", strwaderror(waderrno));
			return ERRORCLEAN_WAD_ERROR + waderrno;
		}
	}
	nextarg(argparser);
	return 0;
}

// If nonzero, bad parse.
static int parse_switches(arg_parser_t *argparser, wadtool_options_clean_t *options)
{
	options->outpath = takearg(argparser);

	if (!options->outpath || strlen(options->outpath) == 0)
	{
		options->outpath = options->filename;
		options->same_output = 1;
	}

	// TODO: Remove me.
	options->verbose = 1;

	return 0;
}

static int call(arg_parser_t *argparser)
{
	wadtool_options_clean_t options = {NULL, NULL, NULL, 0, 0};

	int err;
	if ((err = parse_file(argparser, &options)))
	{
		return err;
	}
	if ((err = parse_switches(argparser, &options)))
	{
		WAD_Close(options.wad);
		return err;
	}
	
	int ret = exec(&options);
	WAD_Close(options.wad);
	return ret;
}

static void usage()
{
	printf("Usage: wad clean [wadfile] <destination>\n");
}

static void help()
{
	printf("[wadfile]: \n");
	printf("    The WAD file to clean.\n");
	printf("\n");
	printf("<destination>: (optional, default same file)\n");
	printf("    The new path to the output file.\n");
}

wadtool_t WADTOOL_Clean = {
	"clean",
	"Rebuilds a WAD file, minus removed or unreferenced data.",
	&call,
	&usage,
	&help,
};
