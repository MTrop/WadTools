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
#include "stream_config.h"
#include "stream.h"

#ifndef min
#define min(x,y) ((x) < (y) ? (x) : (y))
#endif

#ifndef max
#define max(x,y) ((x) > (y) ? (x) : (y))
#endif

// ===========================================================================
// Common Private Functions
// ===========================================================================

static stream_t* STREAM_Init()
{
	stream_t *out = (stream_t*)STREAM_MALLOC(sizeof(stream_t));
	
	out->type = STREAMI_UNKNOWN;
	out->file = NULL;
	out->file_origin_pos = -1;
	out->file_opened = 0;
	
	out->buffer = NULL;
	out->buffer_length = -1;
	out->buffer_content_length = 0;
	out->buffer_pos = -1;

	out->pos = 0;
	out->length = STREAM_NO_LENGTH;
	
	return out;
}

static int STREAM_AllocateBuffer(stream_t *stream, size_t length)
{
	stream->buffer = (unsigned char *)STREAM_MALLOC(sizeof(unsigned char) * length);
	if (!stream->buffer)
		return 1;
	stream->buffer_pos = -1;
	stream->buffer_length = length;
	return 0;
}

static void STREAM_FreeAllocated(stream_t *stream)
{
	STREAM_FREE(stream);
}

static char* STREAM_TypeName(stream_type_t type)
{
	switch (type)
	{
		case STREAMI_FILE: return "File";
		case STREAMI_BUFFER: return "Buffer";
		case STREAMI_UNKNOWN: return "!UNKNOWN!";
	}
	
	return "Unknown";
}

// ===========================================================================
// Virtual function table for implementation handling.
// ===========================================================================

typedef struct {
	
	int    (*destroy)(stream_t*);
	int    (*reset)(stream_t*);
	int    (*get_char)(stream_t*);
	int    (*read_data)(stream_t*, void*, size_t, size_t);
	
} streamfuncs_t;


// ===========================================================================
// STREAMI_FILE
// ===========================================================================

static int streami_file_destroy(stream_t *stream)
{
	if (stream->file_opened)
		if (!fclose(stream->file))
			return 1;
		
	if (stream->buffer)
		STREAM_FREE(stream->buffer);
	
	return 0;
}

static int streami_file_reset(stream_t *stream)
{
	if (fseek(stream->file, stream->file_origin_pos, SEEK_SET))
		return 1;

	if (stream->buffer)
		stream->buffer_pos = -1;
	
	stream->pos = 0;

	return 0;
}

static int streami_file_fill_buffer(stream_t *stream)
{
	// fill buffer if at end.
	if (stream->buffer_pos < 0 || stream->buffer_pos >= stream->buffer_content_length)
	{
		size_t buf;
		if (stream->length == STREAM_NO_LENGTH)
			buf = fread(stream->buffer, 1, stream->buffer_length, stream->file);
		else
			buf = fread(stream->buffer, 1, min(stream->length - stream->pos, stream->buffer_length), stream->file);
		
		if (!buf)
			return EOF;
		else
		{
			stream->buffer_content_length = buf;
			stream->buffer_pos = 0;
		}
	}
	
	return stream->buffer_content_length - stream->buffer_pos;
}

static int streami_file_get_char(stream_t *stream)
{
	int out;
	if (stream->buffer)
	{
		int buf = streami_file_fill_buffer(stream);
		if (buf == EOF)
			return EOF;
		out = stream->buffer[stream->buffer_pos++] & 0x0FF;
	}
	else
	{
		if (stream->pos >= stream->length)
			return EOF;
		out = fgetc(stream->file);
		if (out == EOF)
			return EOF;
	}
		
	stream->pos++;
	return out;
}

static int streami_file_read_data(stream_t *stream, void *destination, size_t size, size_t count)
{
	int out = 0;
	unsigned char *ptr = destination;
	
	if (stream->buffer)
	{
		while (count--)
		{
			size_t copied = 0;
			while (copied < size)
			{
				// fill buffer if at end.
				if (streami_file_fill_buffer(stream) == EOF)
					return out;
				
				// size is greater than what's left.
				size_t amount = min(stream->buffer_content_length - stream->buffer_pos, size - copied);
				memcpy(ptr, &(stream->buffer[stream->buffer_pos]), amount);
				stream->buffer_pos += amount;
				stream->pos += amount;
				copied += amount;
				ptr += amount;
			}
			out++;
		}
	}
	else
	{
		out = fread(destination, size, count, stream->file);
		stream->pos += size * count;
	}

	return out;
}

static streamfuncs_t STREAMI_FILE_STREAMFUNCS = {
	streami_file_destroy,
	streami_file_reset,
	streami_file_get_char,
	streami_file_read_data,
};

// ===========================================================================
// STREAMI_BUFFER
// ===========================================================================

static int streami_buffer_destroy(stream_t *stream)
{
	// Nothing to destroy. DO NOT FREE THE BUFFER - it was encapsulated.
	return 0;
}

static int streami_buffer_reset(stream_t *stream)
{
	stream->buffer_pos = 0;
	stream->pos = 0;
	return 0;
}

static int streami_buffer_get_char(stream_t *stream)
{
	if (stream->buffer_pos >= stream->buffer_length)
		return EOF;
	
	stream->pos++;
	return stream->buffer[stream->buffer_pos++];
}

static int streami_buffer_read_data(stream_t *stream, void *destination, size_t size, size_t count)
{
	int out = 0;
	unsigned char *ptr = destination;

	while (count--)
	{
		if (stream->buffer_content_length - stream->buffer_pos < size)
		{
			memcpy(ptr, &(stream->buffer[stream->buffer_pos]), stream->buffer_content_length - stream->buffer_pos);
			stream->buffer_pos = stream->buffer_content_length;
			return out;
		}
		else
		{
			memcpy(ptr, &(stream->buffer[stream->buffer_pos]), size);
			stream->buffer_pos += size;
			out++;
		}
	}
	
	return out;
}

static streamfuncs_t STREAMI_BUFFER_STREAMFUNCS = {
	streami_buffer_destroy,
	streami_buffer_reset,
	streami_buffer_get_char,
	streami_buffer_read_data,
};

// ...........................................................................

static streamfuncs_t* STREAM_funcs(stream_type_t type)
{
	switch (type)
	{
		case STREAMI_FILE: return &STREAMI_FILE_STREAMFUNCS;
		case STREAMI_BUFFER: return &STREAMI_BUFFER_STREAMFUNCS;
		case STREAMI_UNKNOWN: return NULL;
	}
	
	return NULL;
}

#define STREAMI_FUNC(s,f) (STREAM_funcs((s)->type))->f


// ===========================================================================
// Public Functions
// ===========================================================================

// ---------------------------------------------------------------
// stream_t* STREAM_Open(char *filename)
// See stream.h
// ---------------------------------------------------------------
stream_t* STREAM_Open(char *filename)
{
	FILE *fp = fopen(filename, "rb");
	if (!fp)
		return NULL;
	
	stream_t *out;
	if (!(out = STREAM_OpenFile(fp)))
		return NULL;

	out->file_opened = 1;
	
	return out;
}

// ---------------------------------------------------------------
// stream_t* STREAM_OpenBufferedFile(char *filename, int buffer_size)
// See stream.h
// ---------------------------------------------------------------
stream_t* STREAM_OpenBuffered(char *filename, int buffer_size)
{
	FILE *fp = fopen(filename, "rb");
	if (!fp)
		return NULL;

	stream_t *out;
	if (!(out = STREAM_OpenBufferedFile(fp, buffer_size)))
		return NULL;
	
	out->file_opened = 1;
	
	return out;
}

// ---------------------------------------------------------------
// stream_t* STREAM_OpenFile(FILE *file)
// See stream.h
// ---------------------------------------------------------------
stream_t* STREAM_OpenFile(FILE *file)
{
	long int cur = ftell(file);
	if (fseek(file, 0, SEEK_END))
		return STREAM_OpenFileSection(file, STREAM_NO_LENGTH);

	long int end = ftell(file);
	fseek(file, cur, SEEK_SET);
	return STREAM_OpenFileSection(file, end - cur);
}

// ---------------------------------------------------------------
// stream_t* STREAM_OpenBufferedFile(FILE *file, int buffer_size)
// See stream.h
// ---------------------------------------------------------------
stream_t* STREAM_OpenBufferedFile(FILE *file, int buffer_size)
{
	long int cur = ftell(file);
	if (fseek(file, 0, SEEK_END))
		return STREAM_OpenBufferedFileSection(file, STREAM_NO_LENGTH, buffer_size);
		
	long int end = ftell(file);
	fseek(file, cur, SEEK_SET);
	return STREAM_OpenBufferedFileSection(file, end - cur, buffer_size);
}

// ---------------------------------------------------------------
// stream_t* STREAM_OpenFileSection(FILE *file, size_t length)
// See stream.h
// ---------------------------------------------------------------
stream_t* STREAM_OpenFileSection(FILE *file, size_t length)
{
	stream_t *out = STREAM_Init();
	if (!out)
		return NULL;
	
	out->file = file;
	out->file_origin_pos = ftell(file);

	out->pos = 0;
	out->length = length;
	
	out->type = STREAMI_FILE;
	return out;
}

// ---------------------------------------------------------------
// stream_t* STREAM_OpenBufferedFileSection(FILE *file, size_t length, int buffer_size)
// See stream.h
// ---------------------------------------------------------------
stream_t* STREAM_OpenBufferedFileSection(FILE *file, size_t length, int buffer_size)
{
	stream_t *out = STREAM_Init();
	// make sure valid size.
	buffer_size = buffer_size < 1 ? 1 : buffer_size;
	if (STREAM_AllocateBuffer(out, buffer_size))
	{
		STREAM_FreeAllocated(out);
		return NULL;
	}
	
	out->file = file;
	out->file_origin_pos = ftell(file);
	
	out->pos = 0;
	out->length = length;
	
	out->type = STREAMI_FILE;
	return out;	
}

// ---------------------------------------------------------------
// stream_t* STREAM_OpenBuffer(unsigned char *buffer, size_t length)
// See stream.h
// ---------------------------------------------------------------
stream_t* STREAM_OpenBuffer(unsigned char *buffer, size_t length)
{
	stream_t *out = STREAM_Init();
	if (!out)
		return NULL;
	
	out->type = STREAMI_BUFFER;
	out->buffer = buffer;
	out->buffer_pos = 0;
	out->buffer_content_length = length;
	out->buffer_length = length;
	out->pos = 0;
	out->length = length;

	return out;
}

// ---------------------------------------------------------------
// int STREAM_Reset(stream_t *stream)
// See stream.h
// ---------------------------------------------------------------
int STREAM_Reset(stream_t *stream)
{
	if (stream == NULL)
		return 1;
	
	if ((STREAMI_FUNC(stream, reset))(stream))
		return 1;
	
	return 0;
}

// ---------------------------------------------------------------
// size_t STREAM_Tell(stream_t *stream)
// See stream.h
// ---------------------------------------------------------------
inline size_t STREAM_Tell(stream_t *stream)
{
	return stream->pos;
}

// ---------------------------------------------------------------
// size_t STREAM_Length(stream_t *stream)
// See stream.h
// ---------------------------------------------------------------
inline size_t STREAM_Length(stream_t *stream)
{
	return stream->length;
}

// ---------------------------------------------------------------
// int STREAM_GetChar(stream_t *stream)
// See stream.h
// ---------------------------------------------------------------
int STREAM_GetChar(stream_t *stream)
{
	if (stream == NULL)
		return EOF;
	
	return (STREAMI_FUNC(stream, get_char))(stream);
}

// ---------------------------------------------------------------
// int STREAM_ReadLine(stream_t *stream, char *out, int max)
// See stream.h
// ---------------------------------------------------------------
int STREAM_ReadLine(stream_t *stream, char *out, int max)
{
	int amt = 0;
	while (max--)
	{
		int c = STREAM_GetChar(stream);
		if (c == EOF)
		{
			if (!amt)
				return EOF;
			
			*out = '\0';
			return amt;
		}
		
		if (c == 0x0D) // CR
			continue;
		
		if (c == 0x0A) // LF
		{
			*out = '\0';
			return amt;
		}

		*out = (char)(c & 0xFF);
		out++;
		amt++;
	}
	return amt;
}

// ---------------------------------------------------------------
// int STREAM_Get(stream_t *stream, unsigned char *out, int max)
// See stream.h
// ---------------------------------------------------------------
int STREAM_Get(stream_t *stream, unsigned char *out, int max)
{
	int amt = 0;
	while (max--)
	{
		int c = STREAM_GetChar(stream);
		if (c == EOF) // CR
			return amt;
		*out = (unsigned char)(c & 0xFF);
		out++;
		amt++;
	}
	return amt;
}

// ---------------------------------------------------------------
// int STREAM_Read(stream_t *stream, void *destination, size_t size, size_t count)
// See stream.h
// ---------------------------------------------------------------
int STREAM_Read(stream_t *stream, void *destination, size_t size, size_t count)
{
	if (stream == NULL)
		return -1;
	
	return (STREAMI_FUNC(stream, read_data))(stream, destination, size, count);
}

// ---------------------------------------------------------------
// int STREAM_Close(stream_t *stream)
// See stream.h
// ---------------------------------------------------------------
int STREAM_Close(stream_t *stream)
{
	if (stream == NULL)
		return 0;
	
	if ((STREAMI_FUNC(stream, destroy))(stream))
		return 1;
	
	STREAM_FreeAllocated(stream);
	
	return 0;
}

// ---------------------------------------------------------------
// void STREAM_Dump(stream_t *stream)
// See stream.h
// ---------------------------------------------------------------
void STREAM_Dump(stream_t *stream)
{
	if (stream == NULL)
		return;

	printf("STREAM Type: %s\n", STREAM_TypeName(stream->type));
	printf("\tPos: %d\n", stream->pos);
	printf("\tLength: %d\n", stream->length);
	if (stream->type == STREAMI_FILE)
	{
		printf("FILE\n");
		printf("\tOrigin Pos: %d\n", stream->file_origin_pos);
		printf("\tOpened? %s\n", stream->file_opened ? "YES" : "NO");
	}
	if (stream->type == STREAMI_BUFFER || stream->buffer)
	{
		printf("BUFFER%s\n", stream->type != STREAMI_BUFFER ? " (Backing)" : "");
		printf("\tTotal length: %d\n", stream->buffer_length);
		printf("\tContent length: %d\n", stream->buffer_content_length);
		printf("\tCurrent pos: %d\n", stream->buffer_pos);
	}
}
