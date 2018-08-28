/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include "wadconfig.h"
#include "wad.h"
#include "waderrno.h"

#define min(x,y) ((x) < (y) ? (x) : (y))
#define max(x,y) ((x) > (y) ? (x) : (y))

// Character buffer.
#define CBUF_LEN 16384
static unsigned char cbuf[CBUF_LEN];

// From waderror.h
extern int waderrno;
// From errno.h
extern int errno;

// ===========================================================================
// Common Private Functions
// ===========================================================================

// Create WAD file.
static wad_t* wad_init()
{
	wad_t *out = (wad_t*)WAD_MALLOC(sizeof(wad_t));
	
	out->type = WI_UNKNOWN;
	
	out->header.type = WADTYPE_PWAD;
	out->header.entry_count = 0;
	out->header.entry_list_offset = 12;

	// FILE and Buffer pointer is NULL.
	out->handle.buffer = NULL;
	out->buffer_size = 0;
	out->buffer_capacity = 0;
	
	// Nullify entry list.
	out->entries = NULL;
	out->entries_capacity = 0;

	return out;
}

// Open a file handle to a WAD and check if it is correct.
static int wad_setup_open_file_handle(FILE *fp, wadheader_t *header)
{
	int buf;

	fseek(fp, 0, SEEK_SET);
	buf = fread(header, 1, WADHEADER_LEN, fp);

	// Check file size. Smaller than header = not a WAD.
	if (buf < WADHEADER_LEN)
	{
		fclose(fp);
		return 1;
	}
	
	// Check file type.
	if (WADTYPE_PWAD != header->type && WADTYPE_IWAD != header->type)
	{
		fclose(fp);
		return 1;
	}

	return 0;
}

// Expands/reallocates the internal entry list in a wad_t, if new size is greater than the current size.
static int wad_expand_entrylist(wad_t *wad, int newsize)
{
	int i;
	wadentry_t **oldarray;
	int oldsize = wad->entries_capacity;
	
	if (newsize <= oldsize)
		return 0;

	// Allocate more.
	// Copy current pointers.
	oldarray = wad->entries;
	wad->entries = (wadentry_t**)WAD_MALLOC((sizeof (wadentry_t*)) * newsize);
	
	// if OOM
	if (!(wad->entries))
	{
		wad->entries = oldarray;
		return 1;
	}
	
	if (oldarray)
	{
		// Copy old pointers, delete old array of pointers.
		memcpy(wad->entries, oldarray, (sizeof (wadentry_t*)) * oldsize);
		WAD_FREE(oldarray);
	}

	for (i = oldsize; i < newsize; i++)
	{
		wad->entries[i] = (wadentry_t*)WAD_MALLOC(sizeof (wadentry_t));
		// if OOM
		if (!wad->entries[i])
			return 1;
		wad->entries_capacity = i;
	}

	return 0;
}

// Expands/reallocates the internal data buffer in a wad_t, if new size is greater than the current capacity.
static int wad_expand_buffer(wad_t *wad, int newsize)
{
	int i;
	unsigned char *oldarray;
	int oldsize = wad->buffer_capacity;
	
	if (newsize <= oldsize)
		return 0;
	
	// Copy current data.
	oldarray = wad->handle.buffer;
	wad->handle.buffer = (unsigned char *)WAD_MALLOC((sizeof (unsigned char)) * newsize);
	if (!(wad->handle.buffer))
	{
		wad->handle.buffer = oldarray;
		return 1;
	}		

	if (oldarray)
	{
		memcpy(wad->handle.buffer, oldarray, (sizeof (unsigned char)) * oldsize);
		WAD_FREE(oldarray);
	}

	wad->buffer_capacity = newsize;
	
	return 0;
}


// Load a WAD file's entries.
static int wad_setup_build_entrylist(FILE *fp, wad_t *wad)
{
	int i, count = wad->header.entry_count;
	
	if (wad_expand_entrylist(wad, count))
		return 1;

	// Seek to entry list.
	fseek(fp, wad->header.entry_list_offset, SEEK_SET);
	
	for (i = 0; i < count; i++)
	{
		fread(wad->entries[i], WADENTRY_LEN, 1, fp);
		wad->entries[i]->name[8] = 0x00; // null-terminate name if 8 chars long.
	}
	
	return 0;
}

// Loads the contents of a WAD file into the buffer handle.
static int wad_setup_build_buffer(FILE *fp, wad_t *wad)
{
	int i;
	int len = (wad->header.entry_list_offset) - WADHEADER_LEN;
	int remain = len;
	int amount;
	
	if (wad_expand_buffer(wad, len))
		return 1;
	
	unsigned char *bufptr = wad->handle.buffer;
	
	// Seek to content.
	fseek(fp, WADHEADER_LEN, SEEK_SET);
	while (remain)
	{
		amount = min(CBUF_LEN, remain);
		if (!fread(bufptr, 1, amount, fp))
			return 1;
		remain -= amount;
		bufptr += amount;
	}
	
	wad->buffer_size = len;
	
	return 0;
}

// Frees allocated data in a wad_t
static void wad_free_allocated(wad_t *wad)
{
	int i;
	for (i = 0; i < wad->entries_capacity; i++)
		WAD_FREE(wad->entries[i]);
	WAD_FREE(wad->entries);
	WAD_FREE(wad);
}

// Copies an entry name.
static int wad_entry_name_copy(const char *src, char *dest)
{
	int i = 0;
	while (i < 8)
	{
		char c = src[i];
		if (!c)
			break;
		
		if (c > 0x20 && c < 0x7F)
		{
			// upper-case the letter.
			if (c >= 'a' && c <= 'z')
				dest[i] = c & (~0x20);
			else
				dest[i] = c;
		}
		else
			dest[i] = '_';
		
		i++;
	}
	
	return i;
}

// Adds an entry.
static wadentry_t* wad_add_entry_common(wad_t *wad, const char *name, int32_t length, uint32_t offset, int index)
{
	index = min(wad->header.entry_count, index);
	
	if (index >= wad->entries_capacity)
		if (wad_expand_entrylist(wad, wad->entries_capacity * 2))
			return NULL;
	
	wadentry_t *swap = wad->entries[wad->header.entry_count];

	int i = wad->header.entry_count;
	while (i >= index)
	{
		wad->entries[i] = wad->entries[i - 1];
		i--;
	}

	swap->name[wad_entry_name_copy(name, swap->name)] = 0;
	swap->length = length;
	swap->offset = offset;

	wad->entries[index] = swap;
	wad->header.entry_count++;
	return wad->entries[index];
}

// ===========================================================================
// Virtual function table for implementation handling.
// ===========================================================================

typedef struct {
	
	int         (*destroy)(wad_t*);
	int         (*commit_entries)(wad_t*);
	wadentry_t* (*create_entry_at)(wad_t*, const char*, int);
	wadentry_t* (*add_entry_at)(wad_t*, const char*, int, unsigned char*, size_t);
	wadentry_t* (*add_entry_data_at)(wad_t*, const char*, int, FILE*);
	int         (*remove_entry_at)(wad_t*, int);
	int         (*read_data)(wad_t*, wadentry_t*, void*, size_t, size_t);
	
} wadfuncs_t;


// ===========================================================================
// WI_MAP
// ===========================================================================

static int wi_map_destroy(wad_t *wad)
{
	// Do nothing. Entry cleanup done in another function.
	return 0;
}

static int wi_map_commit_entries(wad_t *wad)
{
	// Do nothing. Nothing to write to.
	return 0;
}

static wadentry_t* wi_map_create_entry_at(wad_t *wad, const char *name, int index)
{
	// Not supported.
	waderrno = WADERROR_NOT_SUPPORTED;
	return NULL;
}

static wadentry_t* wi_map_add_entry_at(wad_t *wad, const char *name, int index, unsigned char *buffer, size_t size)
{
	// Not supported.
	waderrno = WADERROR_NOT_SUPPORTED;
	return NULL;
}

static wadentry_t* wi_map_add_entry_data_at(wad_t *wad, const char *name, int index, FILE *stream)
{
	// Not supported.
	waderrno = WADERROR_NOT_SUPPORTED;
	return NULL;
}

static wadfuncs_t WI_MAP_WADFUNCS = {
	wi_map_destroy,
	wi_map_commit_entries,
	wi_map_create_entry_at,
	wi_map_add_entry_at,
	wi_map_add_entry_data_at,
};

// ===========================================================================
// WI_FILE
// ===========================================================================

static int wi_file_destroy(wad_t *wad)
{
	// Close file handle.
	fflush(wad->handle.file);
	fclose(wad->handle.file);
	return 0;
}

static int wi_file_commit_header(wad_t *wad)
{
	errno = 0;
	FILE *file = wad->handle.file;
	fseek(file, 0, SEEK_SET);
	if (!fwrite(&(wad->header), WADHEADER_LEN, 1, file))
		return 1;

	return 0;
}

static int wi_file_commit_entry(wad_t *wad, int index)
{
	errno = 0;
	FILE *file = wad->handle.file;
	fseek(file, wad->header.entry_list_offset + (WADENTRY_LEN * index), SEEK_SET);
	if (!fwrite(wad->entries[index], WADENTRY_LEN, 1, file))
		return 1;
	
	return 0;
}

static int wi_file_commit_entries(wad_t *wad)
{
	if (wi_file_commit_header(wad))
	{
		if (errno)
			waderrno = WADERROR_FILE_ERROR;
		else
			waderrno = WADERROR_CANNOT_COMMIT;
		return 1;
	}
	
	int i = 0;
	while (i < wad->header.entry_count)
	{
		if (wi_file_commit_entry(wad, i++))
		{
			if (errno)
				waderrno = WADERROR_FILE_ERROR;
			else
				waderrno = WADERROR_CANNOT_COMMIT;
			return 1;
		}
	}

	return 0;
}

static wadentry_t* wi_file_create_entry_at(wad_t *wad, const char *name, int index)
{
	wadentry_t* entry;
	if (!(entry = wad_add_entry_common(wad, name, 0, 0, index)))
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	if (wi_file_commit_entries(wad))
		return NULL;
	return entry;
}

static wadentry_t* wi_file_add_entry_at(wad_t *wad, const char *name, int index, unsigned char *buffer, size_t size)
{
	int amount;
	wadentry_t* entry;
	int pos = wad->header.entry_list_offset;
	if (!(entry = wad_add_entry_common(wad, name, size, pos, index)))
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	
	FILE *fp = wad->handle.file;
	fseek(fp, pos, SEEK_SET);
	
	size_t remain = size;
	
	while (remain)
	{
		amount = min(CBUF_LEN, remain);
		if (fwrite(buffer, 1, amount, fp) < amount)
		{
			waderrno = WADERROR_FILE_ERROR;
			return NULL;
		}
		remain -= amount;
		buffer += amount;
	}
	
	wad->header.entry_list_offset = pos + size;
	
	if (wi_file_commit_entries(wad))
		return NULL;
	
	return entry;
}

static wadentry_t* wi_file_add_entry_data_at(wad_t *wad, const char *name, int index, FILE *stream)
{
	wadentry_t* entry;
	int pos = wad->header.entry_list_offset;
	FILE *fp = wad->handle.file;
	fseek(fp, pos, SEEK_SET);
	
	int buf = 0;
	int amount = 0;
	while ((buf = fread(cbuf, 1, CBUF_LEN, stream)))
	{
		if (fwrite(cbuf, 1, buf, fp) < buf)
		{
			waderrno = WADERROR_FILE_ERROR;
			return NULL;
		}
		amount += buf;
	}
	
	wad->header.entry_list_offset = pos + amount;

	if (!(entry = wad_add_entry_common(wad, name, amount, pos, index)))
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	
	if (wi_file_commit_entries(wad))
		return NULL;
	
	return entry;
}

static wadfuncs_t WI_FILE_WADFUNCS = {
	wi_file_destroy,
	wi_file_commit_entries,
	wi_file_create_entry_at,
	wi_file_add_entry_at,
	wi_file_add_entry_data_at,
};

// ===========================================================================
// WI_BUFFER
// ===========================================================================

static int wi_buffer_destroy(wad_t *wad)
{
	// Free buffer itself.
	WAD_FREE(wad->handle.buffer);
	wad->buffer_size = 0;
	wad->buffer_capacity = -1;
	return 0;
}

static int wi_buffer_commit_entries(wad_t *wad)
{
	// Do nothing. Nothing to write to.
	return 0;
}

static wadentry_t* wi_buffer_create_entry_at(wad_t *wad, const char *name, int index)
{
	wadentry_t* entry;
	if (!(entry = wad_add_entry_common(wad, name, 0, 0, index)))
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	return entry;
}

static int wi_buffer_attempt_expand(wad_t *wad, int add)
{
	// expand buffer.
	if (wad->buffer_size + add >= wad->buffer_capacity)
	{
		if (wad_expand_buffer(wad, wad->buffer_capacity * 2))
			return 1;
	}
	
	return 0;
}

static wadentry_t* wi_buffer_add_entry_at(wad_t *wad, const char *name, int index, unsigned char *buffer, size_t size)
{
	if (wi_buffer_attempt_expand(wad, size))
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	
	unsigned char *dest = &(wad->handle.buffer[wad->buffer_size]);
	memcpy(dest, buffer, size);
	
	wadentry_t* entry;	
	if (!(entry = wad_add_entry_common(wad, name, size, wad->buffer_size, index)))
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	
	wad->buffer_size += size;
	wad->header.entry_list_offset = wad->buffer_size + WADHEADER_LEN;

	return entry;
}

static wadentry_t* wi_buffer_add_entry_data_at(wad_t *wad, const char *name, int index, FILE *stream)
{
	int buf = 0;
	int amount = 0;
	unsigned char *dest = NULL;
	while ((buf = fread(cbuf, 1, CBUF_LEN, stream)))
	{
		if (wi_buffer_attempt_expand(wad, buf))
		{
			waderrno = WADERROR_OUT_OF_MEMORY;
			return NULL;
		}
		
		dest = &(wad->handle.buffer[wad->buffer_size]);
		memcpy(dest, cbuf, buf);
		
		wad->buffer_size += buf;
		amount += buf;
	}
	
	int pos = wad->header.entry_list_offset;
	wad->header.entry_list_offset = pos + amount;

	wadentry_t* entry;
	if (!(entry = wad_add_entry_common(wad, name, amount, pos, index)))
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	
	return entry;
}

static wadfuncs_t WI_BUFFER_WADFUNCS = {
	wi_buffer_destroy,
	wi_buffer_commit_entries,
	wi_buffer_create_entry_at,
	wi_buffer_add_entry_at,
	wi_buffer_add_entry_data_at,
};

// ...........................................................................

static wadfuncs_t* wad_funcs(wadimpl_t impl)
{
	switch (impl)
	{
		case WI_MAP: return &WI_MAP_WADFUNCS;
		case WI_FILE: return &WI_FILE_WADFUNCS;
		case WI_BUFFER: return &WI_BUFFER_WADFUNCS;
	}
	
	return NULL;
}

#define WI_FUNC(w,f) (wad_funcs((w)->type))->f

// ===========================================================================
// Public Functions
// ===========================================================================

// ---------------------------------------------------------------
// wad_t* wad_open(char *filename)
// See wad.h
// ---------------------------------------------------------------
wad_t* wad_open(char *filename)
{
	wad_t *out;
	FILE *fp;

	// Reset error state.
	waderrno = WADERROR_NO_ERROR;
	
	fp = fopen(filename, "r+b");
	if (!fp)
	{
		waderrno = WADERROR_FILE_ERROR;
		return NULL;
	}

	out = wad_init();
	if (!out)
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	if (wad_setup_open_file_handle(fp, &(out->header)))
	{
		waderrno = WADERROR_FILE_NOT_A_WAD;
		return NULL;
	}
	if (wad_setup_build_entrylist(fp, out))
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}

	out->type = WI_FILE;
	out->handle.file = fp;	
	
	return out;
}

// ---------------------------------------------------------------
// wad_t* wad_create(char *filename)
// See wad.h
// ---------------------------------------------------------------
wad_t* wad_create(char *filename)
{
	wad_t *out;
	FILE *fp;

	// Reset error state.
	waderrno = WADERROR_NO_ERROR;

	fp = fopen(filename, "w+b");
	if (!fp)
	{
		waderrno = WADERROR_FILE_ERROR;
		return NULL;
	}

	out = wad_init();
	if (!out)
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	if (wad_expand_entrylist(out, 8))
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}

	out->type = WI_FILE;
	out->handle.file = fp;

	if (wi_file_commit_header(out))
	{
		waderrno = WADERROR_CANNOT_COMMIT;
		return NULL;
	}
	if (wad_setup_open_file_handle(fp, &(out->header)))
	{
		waderrno = WADERROR_FILE_NOT_A_WAD;
		return NULL;
	}

	return out;
}

// ---------------------------------------------------------------
// wad_t* wad_open_map(char *filename)
// See wad.h
// ---------------------------------------------------------------
wad_t* wad_open_map(char *filename)
{
	wad_t *out;
	FILE *fp;

	// Reset error state.
	waderrno = WADERROR_NO_ERROR;

	fp = fopen(filename, "rb");
	if (!fp)
	{
		waderrno = WADERROR_FILE_ERROR;
		return NULL;
	}

	out = wad_init();
	if (!out)
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	if (wad_setup_open_file_handle(fp, &(out->header)))
	{
		waderrno = WADERROR_FILE_NOT_A_WAD;
		return NULL;
	}
	if (wad_setup_build_entrylist(fp, out))
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}

	fclose(fp);
	out->type = WI_MAP;

	return out;
}

// ---------------------------------------------------------------
// wad_t* wad_open_buffer(char *filename)
// See wad.h
// ---------------------------------------------------------------
wad_t* wad_open_buffer(char *filename)
{
	wad_t *out;
	FILE *fp;

	// Reset error state.
	waderrno = WADERROR_NO_ERROR;

	fp = fopen(filename, "rb");
	if (!fp)
	{
		waderrno = WADERROR_FILE_ERROR;
		return NULL;
	}

	out = wad_init();
	if (!out)
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	if (wad_setup_open_file_handle(fp, &(out->header)))
	{
		waderrno = WADERROR_FILE_NOT_A_WAD;
		return NULL;
	}
	if (wad_setup_build_entrylist(fp, out))
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	if (wad_setup_build_buffer(fp, out))
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}

	out->type = WI_BUFFER;

	return out;
}

// ---------------------------------------------------------------
// wad_t* wad_create_buffer()
// See wad.h
// ---------------------------------------------------------------
wad_t* wad_create_buffer()
{
	return wad_create_buffer_init(WADBUFFER_INITSIZE);
}

// ---------------------------------------------------------------
// wad_t* wad_create_buffer_init(int size)
// See wad.h
// ---------------------------------------------------------------
wad_t* wad_create_buffer_init(int size)
{
	wad_t *out;

	// Reset error state.
	waderrno = WADERROR_NO_ERROR;

	out = wad_init();
	if (!out)
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	if (wad_expand_entrylist(out, 16))
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	if (wad_expand_buffer(out, size))
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}

	out->type = WI_BUFFER;

	return out;
}

// ---------------------------------------------------------------
// int wad_entry_count(wad_t *wad)
// See wad.h
// ---------------------------------------------------------------
int wad_entry_count(wad_t *wad)
{	
	// Reset error state.
	waderrno = WADERROR_NO_ERROR;

	if (wad == NULL)
	{
		waderrno = WADERROR_WAD_INVALID;
		return -1;
	}
	
	return (int)(wad->header.entry_count);
}

// ---------------------------------------------------------------
// waditerator_t* wad_iterator_create(wad_t *wad, int start)
// See wad.h
// ---------------------------------------------------------------
waditerator_t* wad_iterator_create(wad_t *wad, int start)
{
	// Reset error state.
	waderrno = WADERROR_NO_ERROR;

	waditerator_t *out = (waditerator_t*)WAD_MALLOC(sizeof(waditerator_t));
	
	if (wad == NULL)
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}

	out->wad = wad;
	out->entry = NULL;
	out->next = start;
	out->count = wad->header.entry_count;
	
	return out;
}

// ---------------------------------------------------------------
// void wad_iterator_reset(waditerator_t *iter, int start)
// See wad.h
// ---------------------------------------------------------------
void wad_iterator_reset(waditerator_t *iter, int start)
{
	iter->entry = NULL;
	iter->next = start;
	iter->count = iter->wad->header.entry_count;
}

// ---------------------------------------------------------------
// wadentry_t* wad_iterator_next(waditerator_t *iter)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* wad_iterator_next(waditerator_t *iter)
{
	if (iter->next < iter->count)
	{
		iter->entry = iter->wad->entries[iter->next];
		iter->next++;
		return iter->entry;
	}
	else
	{
		return NULL;
	}
}

// ---------------------------------------------------------------
// void wad_iterator_close(waditerator_t *iter)
// See wad.h
// ---------------------------------------------------------------
void wad_iterator_close(waditerator_t *iter)
{
	WAD_FREE(iter);
}

// ---------------------------------------------------------------
// int wad_commit_entries(wad_t *wad)
// See wad.h
// ---------------------------------------------------------------
int wad_commit_entries(wad_t *wad)
{
	// Reset error state.
	waderrno = WADERROR_NO_ERROR;

	if (wad == NULL)
	{
		waderrno = WADERROR_WAD_INVALID;
		return 1;
	}

	if ((WI_FUNC(wad, commit_entries))(wad))
	{
		waderrno = WADERROR_CANNOT_COMMIT;
		return 1;
	}
	
	return 0;
}

// ---------------------------------------------------------------
// wadentry_t* wad_get_entry(wad_t *wad, int index)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* wad_get_entry(wad_t *wad, int index)
{
	// Reset error state.
	waderrno = WADERROR_NO_ERROR;

	if (wad == NULL)
	{
		waderrno = WADERROR_WAD_INVALID;
		return NULL;
	}
	
	if (index < 0 || index >= wad->header.entry_count)
	{
		waderrno = WADERROR_INDEX_OUT_OF_RANGE;
		return NULL;
	}

	return wad->entries[index];
}

// ---------------------------------------------------------------
// wadentry_t* wad_get_entry_by_name(wad_t *wad, char *name)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* wad_get_entry_by_name(wad_t *wad, const char *name)
{
	return wad_get_entry_by_name_offset(wad, name, 0);
}

// ---------------------------------------------------------------
// wadentry_t* wad_get_entry_by_name_offset(wad_t *wad, char *name, int start)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* wad_get_entry_by_name_offset(wad_t *wad, const char *name, int start)
{
	// Reset error state.
	waderrno = WADERROR_NO_ERROR;

	if (wad == NULL)
	{
		waderrno = WADERROR_WAD_INVALID;
		return NULL;
	}
	
	if (start < 0)
	{
		waderrno = WADERROR_INDEX_OUT_OF_RANGE;
		return NULL;
	}

	while (start < wad->header.entry_count)
	{
		// if equal
		if (!strcmp(name, wad->entries[start]->name))
			return wad->entries[start];
		start++;
	}

	return NULL;
}

// ---------------------------------------------------------------
// wadentry_t* wad_get_entry_by_name_nth(wad_t *wad, char *name, int nth)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* wad_get_entry_by_name_nth(wad_t *wad, const char *name, int nth)
{
	return wad_get_entry_by_name_offset_nth(wad, name, 0, nth);
}

// ---------------------------------------------------------------
// wadentry_t* wad_get_entry_by_name_offset_nth(wad_t *wad, char *name, int start, int nth)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* wad_get_entry_by_name_offset_nth(wad_t *wad, const char *name, int start, int nth)
{
	// Reset error state.
	waderrno = WADERROR_NO_ERROR;

	if (wad == NULL)
	{
		waderrno = WADERROR_WAD_INVALID;
		return NULL;
	}
	
	if (start < 0)
	{
		waderrno = WADERROR_INDEX_OUT_OF_RANGE;
		return NULL;
	}
	
	while (start < wad->header.entry_count)
	{
		// if equal
		if (!strcmp(name, wad->entries[start]->name))
			if (--nth <= 0)
				return wad->entries[start];
		start++;
	}

	return NULL;
}

// ---------------------------------------------------------------
// wadentry_t* wad_get_last_entry_by_name(wad_t *wad, char *name)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* wad_get_last_entry_by_name(wad_t *wad, const char *name)
{
	// Reset error state.
	waderrno = WADERROR_NO_ERROR;

	if (wad == NULL)
	{
		waderrno = WADERROR_WAD_INVALID;
		return NULL;
	}
	
	int i = wad->header.entry_count - 1;
	while (i >= 0)
	{
		// if equal
		if (!strcmp(name, wad->entries[i]->name))
			return wad->entries[i];
		i--;
	}

	return NULL;
}

// ---------------------------------------------------------------
// int wad_get_entry_count(wad_t *wad, char *name)
// See wad.h
// ---------------------------------------------------------------
int wad_get_entry_count(wad_t *wad, const char *name)
{
	// Reset error state.
	waderrno = WADERROR_NO_ERROR;

	if (wad == NULL)
	{
		waderrno = WADERROR_WAD_INVALID;
		return -1;
	}
	
	int i = 0;
	int out = 0;
	while (i < wad->header.entry_count)
	{
		// if equal
		if (!strcmp(name, wad->entries[i]->name))
			out++;
		i++;
	}

	return i;
}

// ---------------------------------------------------------------
// int wad_get_entry_index(wad_t *wad, char *name)
// See wad.h
// ---------------------------------------------------------------
int wad_get_entry_index(wad_t *wad, const char *name)
{
	return wad_get_entry_index_offset(wad, name, 0);
}

// ---------------------------------------------------------------
// int wad_get_entry_index_offset(wad_t *wad, char *name, int start)
// See wad.h
// ---------------------------------------------------------------
int wad_get_entry_index_offset(wad_t *wad, const char *name, int start)
{
	// Reset error state.
	waderrno = WADERROR_NO_ERROR;

	if (wad == NULL)
	{
		waderrno = WADERROR_WAD_INVALID;
		return -1;
	}
	
	if (start < 0)
	{
		waderrno = WADERROR_INDEX_OUT_OF_RANGE;
		return -1;
	}
	
	while (start < wad->header.entry_count)
	{
		// if equal
		if (!strcmp(name, wad->entries[start]->name))
			return start;
		start++;
	}

	return -1;
}

// ---------------------------------------------------------------
// wad_get_entry_indices(wad_t *wad, const char *name, int *out, int offset, int max)
// See wad.h
// ---------------------------------------------------------------
int wad_get_entry_indices(wad_t *wad, const char *name, int *out, int max)
{
	return wad_get_entry_indices_offset(wad, name, 0, out, max);
}

// ---------------------------------------------------------------
// wad_get_entry_indices_offset(wad_t *wad, const char *name, int start, int *out, int max)
// See wad.h
// ---------------------------------------------------------------
int wad_get_entry_indices_offset(wad_t *wad, const char *name, int start, int *out, int max)
{
	// Reset error state.
	waderrno = WADERROR_NO_ERROR;

	if (wad == NULL)
	{
		waderrno = WADERROR_WAD_INVALID;
		return -1;
	}
	
	if (start < 0)
	{
		waderrno = WADERROR_INDEX_OUT_OF_RANGE;
		return -1;
	}
	
	int i = 0;
	while ((*out = wad_get_entry_index_offset(wad, name, start)) >= 0)
	{
		start = (*out) + 1;
		out += 1;
		i++;
	}
	
	return i;
}

// ---------------------------------------------------------------
// int wad_get_entry_last_index(wad_t *wad, char *name)
// See wad.h
// ---------------------------------------------------------------
int wad_get_entry_last_index(wad_t *wad, const char *name)
{
	// Reset error state.
	waderrno = WADERROR_NO_ERROR;

	if (wad == NULL)
	{
		waderrno = WADERROR_WAD_INVALID;
		return -1;
	}
	
	int i = wad->header.entry_count - 1;
	while (i >= 0)
	{
		// if equal
		if (!strcmp(name, wad->entries[i]->name))
			return i;
		i--;
	}

	return -1;
}

// ---------------------------------------------------------------
// wadentry_t* wad_create_entry(wad_t *wad, char *name)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* wad_create_entry(wad_t *wad, const char *name)
{
	return wad_create_entry_at(wad, name, wad->header.entry_count);
}

// ---------------------------------------------------------------
// wadentry_t* wad_create_entry_at(wad_t *wad, char *name, int index)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* wad_create_entry_at(wad_t *wad, const char *name, int index)
{
	// Reset error state.
	waderrno = WADERROR_NO_ERROR;
	errno = 0;

	if (wad == NULL)
	{
		waderrno = WADERROR_WAD_INVALID;
		return NULL;
	}
	
	wadentry_t *out;
	if (!(out = ((WI_FUNC(wad, create_entry_at))(wad, name, index))))
	{
		// waderrno/errno set in call.
		return NULL;
	}

	return out;
}

// ---------------------------------------------------------------
// wadentry_t* wad_add_entry(wad_t *wad, char *name, unsigned char *buffer, size_t buffer_size)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* wad_add_entry(wad_t *wad, const char *name, unsigned char *buffer, size_t buffer_size)
{
	return wad_add_entry_at(wad, name, wad->header.entry_count, buffer, buffer_size);
}

// ---------------------------------------------------------------
// wadentry_t* wad_add_entry_at(wad_t *wad, char *name, int index, unsigned char *buffer, size_t buffer_size)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* wad_add_entry_at(wad_t *wad, const char *name, int index, unsigned char *buffer, size_t buffer_size)
{
	// Reset error state.
	waderrno = WADERROR_NO_ERROR;
	errno = 0;

	if (wad == NULL)
	{
		waderrno = WADERROR_WAD_INVALID;
		return NULL;
	}
	
	wadentry_t *out;
	if (!(out = ((WI_FUNC(wad, add_entry_at))(wad, name, index, buffer, buffer_size))))
	{
		// waderrno/errno set in call.
		return NULL;
	}

	return out;
}

// ---------------------------------------------------------------
// wadentry_t* wad_add_entry_data(wad_t *wad, const char *name, FILE *stream)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* wad_add_entry_data(wad_t *wad, const char *name, FILE *stream)
{
	return wad_add_entry_data_at(wad, name, wad->header.entry_count, stream);
}

// ---------------------------------------------------------------
// wadentry_t* wad_add_entry_data_at(wad_t *wad, const char *name, int index, FILE *stream)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* wad_add_entry_data_at(wad_t *wad, const char *name, int index, FILE *stream)
{
		// Reset error state.
	waderrno = WADERROR_NO_ERROR;
	errno = 0;

	if (wad == NULL)
	{
		waderrno = WADERROR_WAD_INVALID;
		return NULL;
	}
	
	wadentry_t *out;
	if (!(out = ((WI_FUNC(wad, add_entry_data_at))(wad, name, index, stream))))
	{
		// waderrno/errno set in call.
		return NULL;
	}

	return out;
}

// ---------------------------------------------------------------
// int wad_remove_entry_at(wad_t *wad, int index)
// See wad.h
// ---------------------------------------------------------------
int wad_remove_entry_at(wad_t *wad, int index)
{
	// TODO: Finish this.
	return 1;
}

// ---------------------------------------------------------------
// int wad_get_entry_data(wad_t *wad, wadentry_t *entry, void *destination)
// See wad.h
// ---------------------------------------------------------------
int wad_get_entry_data(wad_t *wad, wadentry_t *entry, unsigned char *destination)
{
	int out = wad_read_entry_data(wad, entry, destination, entry->length, 1);
	return out >= 0 ? out * entry->length : out;
}

// ---------------------------------------------------------------
// int wad_read_entry_data(wad_t *wad, wadentry_t *entry, void *destination, size_t size, size_t count)
// See wad.h
// ---------------------------------------------------------------
int wad_read_entry_data(wad_t *wad, wadentry_t *entry, void *destination, size_t size, size_t count)
{
	// TODO: Finish this.
	return 0;
}

// ---------------------------------------------------------------
// int wad_close(wad_t *wad)
// See wad.h
// ---------------------------------------------------------------
int wad_close(wad_t *wad)
{
	// Reset error state.
	waderrno = WADERROR_NO_ERROR;

	if (wad == NULL)
	{
		waderrno = WADERROR_WAD_INVALID;
		return 1;
	}
	
	if ((WI_FUNC(wad, destroy))(wad))
	{
		waderrno = WADERROR_CANNOT_CLOSE;
		return 1;
	}
	
	wad_free_allocated(wad);
	
	return 0;
}
