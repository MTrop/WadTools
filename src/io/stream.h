/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#ifndef __STREAM_H__
#define __STREAM_H__

#include <stdio.h>

#define STREAM_NO_LENGTH	-1

/**
 * Stream type.
 */
typedef enum {
	
	/** Open file. */
	STREAMI_UNKNOWN,
	/** Open file. */
	STREAMI_FILE,
	/** In-memory buffer. */
	STREAMI_BUFFER,
	
} stream_type_t;

/**
 * Stream implementation.
 */
typedef struct {

	/** Stream type. */
	stream_type_t type;

	/** If STREAM_FILE. */
	FILE *file;
	/** Origin position. */
	size_t file_origin_pos;
	/** If this opened a file. */
	int file_opened;

	/** If STREAM_BUFFER or STREAM_FILE plus buffer. */
	unsigned char *buffer;
	/** Buffer max length. */
	int buffer_length;
	/** Buffer content length. */
	int buffer_content_length;
	/** Current buffer position. */
	int buffer_pos;

	/** Current stream position. */
	size_t pos;
	/** Max stream length. */
	size_t length;
	
} stream_t;

// ================ Common WAD Functions ====================

/**
 * Creates a new stream.
 * @param filename the name of the file to open.
 * @return a new lexer stream or NULL if it couldn't be opened or allocated.
 */
stream_t* STREAM_Open(char *filename);

/**
 * Creates a new stream with a backing buffer.
 * The stream name is the file's name.
 * @param filename the name of the file to open.
 * @return a new stream or NULL if it couldn't be opened or allocated.
 */
stream_t* STREAM_OpenBuffered(char *filename, int buffer_size);

/**
 * Creates a new stream from an open file.
 * The file's current position is saved as the "stream origin."
 * @param name the stream name.
 * @param file the open stream.
 * @return a new stream or NULL if it couldn't be opened or allocated.
 */
stream_t* STREAM_OpenFile(FILE *stream);

/**
 * Creates a new stream from an open file with a backing buffer.
 * The file's current position is saved as the "stream origin."
 * @param file the open stream.
 * @param buffer_size the size of the internal buffer in bytes. Values less than 1 are set to 1.
 * @return a new stream or NULL if it couldn't be opened or allocated.
 */
stream_t* STREAM_OpenBufferedFile(FILE *stream, int buffer_size);

/**
 * Creates a new stream from an open file.
 * The file's current position is saved as the "stream origin."
 * @param file the open stream.
 * @param length the maximum amount of bytes to read from the origin in order to stop.
 * @return a new stream or NULL if it couldn't be opened or allocated.
 */
stream_t* STREAM_OpenFileSection(FILE *stream, size_t length);

/**
 * Creates a new stream from an open file with a backing buffer.
 * The file's current position is saved as the "stream origin."
 * @param file the open stream.
 * @param length the maximum amount of bytes to read from the origin in order to stop.
 * @param buffer_size the size of the internal buffer in bytes. Values less than 1 are set to 1.
 * @return a new stream or NULL if it couldn't be opened or allocated.
 */
stream_t* STREAM_OpenBufferedFileSection(FILE *stream, size_t length, int buffer_size);

/**
 * Creates a new stream from a binary char buffer.
 * @param buffer the stream of bytes.
 * @param length the amount of bytes to read in total.
 * @return a new stream or NULL if it couldn't be opened or allocated.
 */
stream_t* STREAM_OpenBuffer(unsigned char *buffer, size_t length);

/**
 * Resets a stream to the beginning.
 * Some contiguous streams cannot be reset.
 * @param name the stream name.
 * @param charstream the stream of characters.
 * @param length the amount of characters.
 * @return 0 if successful, or nonzero if not.
 */
int STREAM_Reset(stream_t *stream);

/**
 * Gets how many bytes have been read so far.
 * @param stream the stream.
 * @return the total amount of bytes read.
 */
size_t STREAM_Tell(stream_t *stream);

/**
 * Get how many bytes will be read in total.
 * Streams may not have a max.
 * @param stream the stream.
 * @return the total amount of bytes that will be read, or STREAM_NO_LENGTH if there's no measured end.
 */
size_t STREAM_Length(stream_t *stream);

/**
 * Gets a single character from the stream.
 * @param stream the stream.
 * @return the character read cast as an integer or EOF and end of stream.
 */
int STREAM_GetChar(stream_t *stream);

/**
 * Gets a full line from a stream, minus the newline (or carriage return).
 * Character output is null-terminated if max is not reached.
 * @param stream the stream.
 * @param out the output line. 
 * @param max the max amount of characters to read.
 * @return the amount of characters read, or -1 if EOF on the read.
 */
int STREAM_ReadLine(stream_t *stream, char *out, int max);

/**
 * Reads a set of bytes from the stream.
 * @param stream the stream.
 * @param out the output buffer.
 * @param max the max amount of characters to read.
 * @return the amount of characters read, or -1 if EOF on the read.
 */
int STREAM_Get(stream_t *stream, unsigned char *out, int max);

/**
 * Reads a set of records from the stream.
 * @param stream the stream.
 * @param destination the destination buffer.
 * @param size the size of a single element in bytes.
 * @param count the amount of elements to read.
 * @return the amount of elements read, or -1 on a read error.
 */
int STREAM_Read(stream_t *stream, void *destination, size_t size, size_t count);

/**
 * Closes a stream.
 * If the stream had to open a file to read from it, the file will be closed.
 * If a backing buffer was created, it too will be freed.
 * If this is a buffer encapsulation (via STREAM_OpenBuffer()), the encapsulated buffer will NOT BE FREED.
 * If successful, the stream pointer is invalidated.
 * @param stream the stream.
 * @return 0 if successful and the stream pointer was invalidated, nonzero if not.
 */
int STREAM_Close(stream_t *stream);

/**
 * Dumps info about a stream to STDOUT.
 * @param stream the stream.
 */
void STREAM_Dump(stream_t *stream);


#endif