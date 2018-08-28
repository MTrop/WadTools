/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/
 
#ifndef __WAD_H__
#define __WAD_H__
 
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// Important defines.
#define WADHEADER_LEN 12
#define WADENTRY_LEN 16
#define WADTYPE_IWAD 0x44415749
#define WADTYPE_PWAD 0x44415750

#define WADBUFFER_INITSIZE (1024 * 32)
#define WADENTRIES_INITSIZE 16

/**
 * A WAD header structure (the start of all WAD files).
 */
typedef struct {
	
	/** The magic number ("IWAD" or "PWAD"). */
	uint32_t type;
	/** The amount of entries in the WAD. */
	int32_t entry_count;
	/** The byte offset into the WAD for the entry list. */
	int32_t entry_list_offset;
	
} wadheader_t;

/**
 * A WAD entry.
 */
typedef struct {
	
	/** Entry content offset into file. */
	uint32_t offset;
	/** Entry content length. */
	int32_t length;
	/** Entry name. */
	unsigned char name[9]; // 8 + null - last byte never read nor written.
	
} wadentry_t;

/**
 * WAD implementation type.
 * This determines how data is loaded and manipulated and what functions to call.
 */
typedef enum {
	
	/** Unknown type - not initialized. */
	WI_UNKNOWN,
	/** Shallow entry map - preview only (no deep read, no write). */
	WI_MAP,
	/** Content and entries stored in memory. */
	WI_BUFFER,
	/** Content and entries read from open file (random access). */
	WI_FILE,
	
} wadimpl_t;

/**
 * Union of implementation handles.
 * If WI_FILE, file is not NULL.
 * If WI_BUFFER, buffer is not NULL.
 * If WI_MAP, neither are used.
 */
typedef union {
	
	/** If WI_FILE. */
	FILE *file;
	/** If WI_BUFFER. */
	unsigned char *buffer;
	
} waddata_u;

/**
 * A WAD abstraction.
 */
typedef struct {

	/** Implementation type. */
	wadimpl_t type;
	/** Handle union. */
	waddata_u handle;
	/** WAD header. */
	wadheader_t header;
	/** WAD entry list. */
	wadentry_t **entries;

	/** WAD entry list current capacity. */
	int entries_capacity;
	/** WAD buffer size (if buffer implementation). */
	int buffer_size;
	/** WAD buffer capacity (if buffer implementation). */
	int buffer_capacity;
	
} wad_t;

/**
 * A simple iterator abstraction for WADs.
 */
typedef struct {
	
	/** Pointer to WAD. */
	wad_t *wad;
	/** Pointer to current entry. */
	wadentry_t *entry;
	/** Next index. */
	int next;
	/** Total count. */
	int count;
	
} waditerator_t;

// ================ Common WAD Functions ====================

/**
 * Opens an existing WAD file for random access.
 * @param filename the file name to open.
 * @return a newly-allocated wad_t (file implementation), or NULL on error.
 */
wad_t* wad_open(char *filename);

/**
 * Creates a new WAD file for random access.
 * The file is created at the specified path.
 * @param filename the file name to create.
 * @return a newly-allocated wad_t (file implementation), or NULL on error.
 */
wad_t* wad_create(char *filename);

/**
 * Opens an existing WAD file, but only skims for entry data.
 * The file is not left open - no handle is kept.
 * @param filename the file name to open.
 * @return a newly-allocated wad_t (mapping implementation), or NULL on error.
 */
wad_t* wad_open_map(char *filename);

/**
 * Creates a WAD buffer in memory by loading the contents of an existing WAD file completely into memory.
 * The file is not left open - no handle is kept.
 * @param filename the file name to open.
 * @return a newly-allocated wad_t (mapping implementation), or NULL on error.
 */
wad_t* wad_open_buffer(char *filename);

/**
 * Creates a WAD buffer in memory with a default initial content buffer size WADBUFFER_INITSIZE.
 * WARNING: Buffers must be saved to disk, or they are not persisted anywhere!
 * @return a newly-allocated wad_t (buffer implementation), or NULL on error.
 */
wad_t* wad_create_buffer();

/**
 * Creates a WAD buffer in memory with a specific initial content buffer size.
 * @param size the initial size, in bytes. 
 * @return a newly-allocated wad_t (mapping implementation), or NULL on error.
 */
wad_t* wad_create_buffer_init(int size);

/**
 * Gets the amount of entries that this WAD contains.
 * @param wad the pointer to the open WAD.
 * @return the amount of entries or -1 on error.
 */
int wad_entry_count(wad_t *wad);

/**
 * Creates a WAD iterator.
 * Must have "next" called on it for the first entry.
 * @param wad the pointer to the open WAD.
 * @param start the starting index into the entry list.
 * @return the amount of entries or -1 on error.
 */
waditerator_t* wad_iterator_create(wad_t *wad, int start);

/**
 * Resets a WAD iterator.
 * Must have "next" called on it for the first entry.
 * @param wad the pointer to the open WAD.
 * @param start the starting index into the entry list.
 * @return the amount of entries or -1 on error.
 */
void wad_iterator_reset(waditerator_t *iter, int start);

/**
 * Advances a WAD iterator to the next entry.
 * @param iter pointer to the iterator.
 * @return a pointer to the next entry.
 */
wadentry_t* wad_iterator_next(waditerator_t *iter);

/**
 * Frees a WAD iterator to the next entry.
 * @param iter pointer to the iterator.
 * @return a pointer to the next entry.
 */
void wad_iterator_close(waditerator_t *iter);

/**
 * Commits changes to WAD entries that would not ordinarily be
 * detected by other functions that manipulate entries.
 * @param wad the pointer to the open WAD.
 * @return 0 if committed properly, nonzero on error.
 */
int wad_commit_entries(wad_t *wad);

/**
 * Gets a WAD entry at a particular index.
 * @param wad the pointer to the open WAD.
 * @param index the entry index to get.
 * @return a valid pointer, or NULL if no corresponding index.
 */
wadentry_t* wad_get_entry(wad_t *wad, int index);

/**
 * Gets the first WAD entry by a particular name.
 * Names are case-sensitive.
 * @param wad the pointer to the open WAD.
 * @param name the entry name.
 * @return a valid pointer, or NULL if no corresponding entry.
 */
wadentry_t* wad_get_entry_by_name(wad_t *wad, const char *name);

/**
 * Gets the first WAD entry by a particular name, from a starting index.
 * Names are case-sensitive.
 * @param wad the pointer to the open WAD.
 * @param name the entry name.
 * @param start the index to start from.
 * @return a valid pointer, or NULL if no corresponding entry.
 */
wadentry_t* wad_get_entry_by_name_offset(wad_t *wad, const char *name, int start);

/**
 * Gets the first WAD entry by a particular name, nth instance.
 * Names are case-sensitive.
 * @param wad the pointer to the open WAD.
 * @param name the entry name.
 * @param nth the nth entry to retrieve (0 or 1 is the first, 2 is second, etc.).
 * @return a valid pointer, or NULL if no corresponding entry.
 */
wadentry_t* wad_get_entry_by_name_nth(wad_t *wad, const char *name, int nth);

/**
 * Gets the first WAD entry by a particular name, nth instance.
 * Names are case-sensitive.
 * @param wad the pointer to the open WAD.
 * @param name the entry name.
 * @param start the index to start from.
 * @param nth the nth entry to retrieve (0 or 1 is the first, 2 is second, etc.).
 * @return a valid pointer, or NULL if no corresponding entry.
 */
wadentry_t* wad_get_entry_by_name_offset_nth(wad_t *wad, const char *name, int start, int nth);

/**
 * Gets the last WAD entry by a particular name (backwards search).
 * Names are case-sensitive.
 * @param wad the pointer to the open WAD.
 * @param name the entry name.
 * @return a valid pointer, or NULL if no corresponding entry.
 */
wadentry_t* wad_get_last_entry_by_name(wad_t *wad, const char *name);

/**
 * Counts how many entries have this name.
 * Names are case-sensitive.
 * @param wad the pointer to the open WAD.
 * @param name the entry name.
 * @return the amount of entries.
 */
int wad_get_entry_count(wad_t *wad, const char *name);

/**
 * Gets the index of a particular entry.
 * Names are case-sensitive.
 * @param wad the pointer to the open WAD.
 * @param name the entry name.
 * @return the matching index or -1 if not found.
 */
int wad_get_entry_index(wad_t *wad, const char *name);

/**
 * Gets the index of a particular entry, from a starting index.
 * Names are case-sensitive.
 * @param wad the pointer to the open WAD.
 * @param name the entry name.
 * @param start the index to start from.
 * @return the matching index or -1 if not found.
 */
int wad_get_entry_index_offset(wad_t *wad, const char *name, int start);

/**
 * Gets the indices of all matching lumps by name.
 * Names are case-sensitive.
 * @param wad the pointer to the open WAD.
 * @param name the entry name.
 * @param out the output array for the indices.
 * @param max the maximum amount of indices to add.
 * @return the amount of indices returned.
 */
int wad_get_entry_indices(wad_t *wad, const char *name, int *out, int max);

/**
 * Gets the indices of all matching lumps by name.
 * Names are case-sensitive.
 * @param wad the pointer to the open WAD.
 * @param name the entry name.
 * @param start the index to start from.
 * @param out the output array for the indices.
 * @param max the maximum amount of indices to add.
 * @return the amount of indices returned.
 */
int wad_get_entry_indices_offset(wad_t *wad, const char *name, int start, int *out, int max);

/**
 * Gets the last index of a particular entry.
 * Names are case-sensitive.
 * @param wad the pointer to the open WAD.
 * @param name the entry name.
 * @return the matching index or -1 if not found.
 */
int wad_get_entry_last_index(wad_t *wad, const char *name);

/**
 * Creates a new WAD entry at the end of the WAD.
 * Bad characters in names are coerced into valid characters.
 * @param wad the pointer to the open WAD.
 * @param name the entry name.
 * @return a pointer to the created entry.
 */
wadentry_t* wad_create_entry(wad_t *wad, const char *name);

/**
 * Creates a new WAD entry at a specific index.
 * The rest of the entries after the index are shifted down a position.
 * Bad characters in names are coerced into valid characters.
 * @param wad the pointer to the open WAD.
 * @param name the entry name.
 * @param index the index position (0-based) to add the entry at.
 * @return a pointer to the created entry.
 */
wadentry_t* wad_create_entry_at(wad_t *wad, const char *name, int index);

/**
 * Adds a new WAD entry (with content) at the end of the WAD.
 * Content is appended to the end of the content blob.
 * Bad characters in names are coerced into valid characters.
 * @param wad the pointer to the open WAD.
 * @param name the entry name.
 * @param buffer the pointer to the data to write.
 * @param buffer_size the amount of data in bytes to write.
 * @return a pointer to the created entry.
 */
wadentry_t* wad_add_entry(wad_t *wad, const char *name, unsigned char *buffer, size_t buffer_size);

/**
 * Adds a new WAD entry (with content) at a specific index.
 * The rest of the entries after the index are shifted down a position.
 * Content is appended to the end of the content blob.
 * Bad characters in names are coerced into valid characters.
 * @param wad the pointer to the open WAD.
 * @param name the entry name.
 * @param index the index position (0-based) to add the entry at.
 * @param buffer the pointer to the data to write.
 * @param buffer_size the amount of data in bytes to write.
 * @return a pointer to the created entry.
 */
wadentry_t* wad_add_entry_at(wad_t *wad, const char *name, int index, unsigned char *buffer, size_t buffer_size);

/**
 * Adds a new WAD entry (with content) at the end of the WAD, from a FILE.
 * The FILE's position is not adjusted - it is read from its current position to the end.
 * Content is appended to the end of the content blob.
 * Bad characters in names are coerced into valid characters.
 * @param wad the pointer to the open WAD.
 * @param name the entry name.
 * @param buffer the pointer to the data to write.
 * @param buffer_size the amount of data in bytes to write.
 * @return a pointer to the created entry.
 */
wadentry_t* wad_add_entry_data(wad_t *wad, const char *name, FILE *stream);

/**
 * Adds a new WAD entry (with content) at a specific index.
 * The FILE's position is not adjusted - it is read from its current position to the end.
 * The rest of the entries after the index are shifted down a position.
 * Content is appended to the end of the content blob.
 * Bad characters in names are coerced into valid characters.
 * @param wad the pointer to the open WAD.
 * @param name the entry name.
 * @param index the index position (0-based) to add the entry at.
 * @param buffer the pointer to the data to write.
 * @param buffer_size the amount of data in bytes to write.
 * @return a pointer to the created entry.
 */
wadentry_t* wad_add_entry_data_at(wad_t *wad, const char *name, int index, FILE *stream);

/**
 * Removes an entry from the WAD (but not its content).
 * The rest of the entries after the index are shifted up a position.
 * @param wad the pointer to the open WAD.
 * @param index the index position (0-based) to remove the entry from.
 * @return 0 if successful, nonzero if not.
 */
int wad_remove_entry_at(wad_t *wad, int index);

/**
 * Gets the content of an entry from a WAD.
 * The destination must be large enough to accomodate the data.
 * @param wad the pointer to the open WAD.
 * @param entry the entry to use for length and offset.
 * @param destination the destination buffer.
 * @return the amount of bytes read, or -1 on a read error.
 */
int wad_get_entry_data(wad_t *wad, wadentry_t *entry, unsigned char *destination);

/**
 * Gets the content of an entry from a WAD.
 * The destination must be large enough to accomodate the data.
 * @param wad the pointer to the open WAD.
 * @param entry the entry to use for length and offset.
 * @param destination the destination buffer.
 * @param size the size of a single element in bytes.
 * @param count the amount of elements to read.
 * @return the amount of elements read, or -1 on a read error.
 */
int wad_read_entry_data(wad_t *wad, wadentry_t *entry, void *destination, size_t size, size_t count);

/**
 * Closes an open WAD, performs flushing operations on it if necessary,
 * then frees it from memory. The pointer provided is then invalid.
 * @param wad the pointer to the open WAD.
 * @return 0 if closed properly and wad was freed, nonzero on error.
 */
int wad_close(wad_t *wad);

#endif
