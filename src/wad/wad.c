/*****************************************************************************
 * Copyright (c) 2018 Matt Tropiano
 * All rights reserved. This source and the accompanying materials
 * are made available under the terms of the GNU Lesser Public License v2.1
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "wadconfig.h"
#include "wad.h"
#include "waderrno.h"

#ifndef min
#define min(x,y) ((x) < (y) ? (x) : (y))
#endif

#ifndef max
#define max(x,y) ((x) > (y) ? (x) : (y))
#endif

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
static wad_t* WAD_Init()
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
static int WAD_SetupOpenFileHandle(FILE *fp, wadheader_t *header)
{
	int buf;

	if (fseek(fp, 0, SEEK_SET))
	{
		fclose(fp);
		return 1;
	}
	
	buf = fread(header, 1, sizeof(wadheader_t), fp);

	// Check file size. Smaller than header = not a WAD.
	if (buf < sizeof(wadheader_t))
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
static int WAD_ExpandEntrylist(wad_t *wad, int newsize)
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
static int WAD_ExpandBuffer(wad_t *wad, int newsize)
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
static int WAD_SetupBuildEntrylist(FILE *fp, wad_t *wad)
{
	int i, count = wad->header.entry_count;
	
	if (WAD_ExpandEntrylist(wad, count))
		return 1;

	// Seek to entry list.
	if (fseek(fp, wad->header.entry_list_offset, SEEK_SET))
		return 1;
	
	for (int i = 0; i < count; i++)
		if (!fread(wad->entries[i], sizeof(wadentry_t), 1, fp))
			return 1;
	
	return 0;
}

// Loads the contents of a WAD file into the buffer handle.
static int WAD_SetupBuildBuffer(FILE *fp, wad_t *wad)
{
	int i;
	int len = (wad->header.entry_list_offset) - sizeof(wadheader_t);
	int remain = len;
	int amount;
	
	if (WAD_ExpandBuffer(wad, len))
		return 1;
	
	unsigned char *bufptr = wad->handle.buffer;
	
	// Seek to content.
	if (fseek(fp, sizeof(wadheader_t), SEEK_SET))
		return 1;
	
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
static void WAD_FreeAllocated(wad_t *wad)
{
	int i;
	for (i = 0; i < wad->entries_capacity; i++)
		WAD_FREE(wad->entries[i]);
	WAD_FREE(wad->entries);
	WAD_FREE(wad);
}

// Copies an entry name.
static int WAD_EntryNameCopy(const char *src, char *dest)
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
static wadentry_t* WAD_AddEntryCommon(wad_t *wad, const char *name, int32_t length, uint32_t offset, int index)
{
	index = min(wad->header.entry_count, index);
	
	if (index >= wad->entries_capacity)
		if (WAD_ExpandEntrylist(wad, wad->entries_capacity * 2))
			return NULL;
	
	wadentry_t *swap = wad->entries[wad->header.entry_count];

	int i = wad->header.entry_count;
	while (i >= index)
	{
		wad->entries[i] = wad->entries[i - 1];
		i--;
	}

	swap->name[WAD_EntryNameCopy(name, swap->name)] = 0;
	swap->length = length;
	swap->offset = offset;

	wad->entries[index] = swap;
	wad->header.entry_count++;
	return wad->entries[index];
}

// Removes an entry. Just the entry - no other data.
static int WAD_RemoveEntryCommon(wad_t *wad, int index)
{
	if (index < 0 || index >= wad->header.entry_count)
	{
		waderrno = WADERROR_INDEX_OUT_OF_RANGE;
		return 1;
	}
	
	memmove(&(wad->entries[index]), &(wad->entries[index+1]), sizeof (wadentry_t*) * (wad->header.entry_count - index - 1));

	wad->header.entry_count--;
	return 0;
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
	int         (*get_data)(wad_t*, wadentry_t*, unsigned char*);
	int         (*read_data)(wad_t*, wadentry_t*, void*, size_t, size_t);
	
} wadfuncs_t;


// ===========================================================================
// WI_MAP
// ===========================================================================

// Implementation of wadfuncs_t.destroy(wad_t*)
static int wi_map_destroy(wad_t *wad)
{
	// Do nothing. Entry cleanup done in another function.
	return 0;
}

// Implementation of wadfuncs_t.commit_entries(wad_t*)
static int wi_map_commit_entries(wad_t *wad)
{
	// Not supported.
	waderrno = WADERROR_NOT_SUPPORTED;
	return 0;
}

// Implementation of wadfuncs_t.create_entry_at(wad_t*, const char*, int)
static wadentry_t* wi_map_create_entry_at(wad_t *wad, const char *name, int index)
{
	// Not supported.
	waderrno = WADERROR_NOT_SUPPORTED;
	return NULL;
}

// Implementation of wadfuncs_t.add_entry_at(wad_t*, const char*, int, unsigned char*, size_t)
static wadentry_t* wi_map_add_entry_at(wad_t *wad, const char *name, int index, unsigned char *buffer, size_t size)
{
	// Not supported.
	waderrno = WADERROR_NOT_SUPPORTED;
	return NULL;
}

// Implementation of wadfuncs_t.wi_map_add_entry_data_at(wad_t*, const char*, int, FILE*)
static wadentry_t* wi_map_add_entry_data_at(wad_t *wad, const char *name, int index, FILE *stream)
{
	// Not supported.
	waderrno = WADERROR_NOT_SUPPORTED;
	return NULL;
}

// Implementation of wadfuncs_t.remove_entry_at(wad_t*, int)
static int wi_map_remove_entry_at(wad_t *wad, int index)
{
	// Not supported.
	waderrno = WADERROR_NOT_SUPPORTED;
	return 1;
}

// Implementation of wadfuncs_t.get_data(wad_t*, wadentry_t*, unsigned char*)
static int wi_map_get_data(wad_t *wad, wadentry_t *entry, unsigned char *destination)
{
	// Not supported.
	waderrno = WADERROR_NOT_SUPPORTED;
	return 0;
}

// Implementation of wadfuncs_t.read_data(wad_t*, wadentry_t*, void*, size_t, size_t)
static int wi_map_read_data(wad_t *wad, wadentry_t *entry, void *destination, size_t size, size_t count)
{
	// Not supported.
	waderrno = WADERROR_NOT_SUPPORTED;
	return 0;
}

static wadfuncs_t WI_MAP_WADFUNCS = {
	wi_map_destroy,
	wi_map_commit_entries,
	wi_map_create_entry_at,
	wi_map_add_entry_at,
	wi_map_add_entry_data_at,
	wi_map_remove_entry_at,
	wi_map_get_data,
	wi_map_read_data,
};

// ===========================================================================
// WI_FILE
// ===========================================================================

// Implementation of wadfuncs_t.destroy(wad_t*)
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
	if (fseek(file, 0, SEEK_SET))
		return 1;
	if (!fwrite(&(wad->header), sizeof(wadheader_t), 1, file))
		return 1;

	return 0;
}

static int wi_file_commit_entry(wad_t *wad, int index)
{
	errno = 0;
	FILE *file = wad->handle.file;
	if (fseek(file, wad->header.entry_list_offset + (sizeof(wadentry_t) * index), SEEK_SET))
		return 1;
	if (!fwrite(wad->entries[index], sizeof(wadentry_t), 1, file))
		return 1;
	
	return 0;
}

// Implementation of wadfuncs_t.commit_entries(wad_t*)
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

// Implementation of wadfuncs_t.create_entry_at(wad_t*, const char*, int)
static wadentry_t* wi_file_create_entry_at(wad_t *wad, const char *name, int index)
{
	wadentry_t* entry;
	if (!(entry = WAD_AddEntryCommon(wad, name, 0, 0, index)))
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	if (wi_file_commit_entries(wad))
		return NULL;
	return entry;
}

// Implementation of wadfuncs_t.add_entry_at(wad_t*, const char*, int, unsigned char*, size_t)
static wadentry_t* wi_file_add_entry_at(wad_t *wad, const char *name, int index, unsigned char *buffer, size_t size)
{
	int amount;
	wadentry_t* entry;
	int pos = wad->header.entry_list_offset;
	if (!(entry = WAD_AddEntryCommon(wad, name, size, pos, index)))
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	
	FILE *fp = wad->handle.file;
	if (fseek(fp, pos, SEEK_SET))
		return NULL;
	
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

// Implementation of wadfuncs_t.wi_map_add_entry_data_at(wad_t*, const char*, int, FILE*)
static wadentry_t* wi_file_add_entry_data_at(wad_t *wad, const char *name, int index, FILE *stream)
{
	wadentry_t* entry;
	int pos = wad->header.entry_list_offset;
	FILE *fp = wad->handle.file;
	if (fseek(fp, pos, SEEK_SET))
	{
		waderrno = WADERROR_FILE_ERROR;
		return NULL;
	}
	
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

	if (!(entry = WAD_AddEntryCommon(wad, name, amount, pos, index)))
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	
	if (wi_file_commit_entries(wad))
		return NULL;
	
	return entry;
}

// Implementation of wadfuncs_t.remove_entry_at(wad_t*, int)
static int wi_file_remove_entry_at(wad_t *wad, int index)
{
	if (WAD_RemoveEntryCommon(wad, index))
		return 1;
	wi_file_commit_entries(wad);
	return 0;
}

// Implementation of wadfuncs_t.get_data(wad_t*, wadentry_t*, unsigned char*)
static int wi_file_get_data(wad_t *wad, wadentry_t *entry, unsigned char *destination)
{
	if (fseek(wad->handle.file, entry->offset, SEEK_SET))
	{
		waderrno = WADERROR_FILE_ERROR;
		return -1;
	}

	return fread(destination, 1, entry->length, wad->handle.file);
}

// Implementation of wadfuncs_t.read_data(wad_t*, wadentry_t*, void*, size_t, size_t)
static int wi_file_read_data(wad_t *wad, wadentry_t *entry, void *destination, size_t size, size_t count)
{
	if (fseek(wad->handle.file, entry->offset, SEEK_SET))
	{
		waderrno = WADERROR_FILE_ERROR;
		return -1;
	}
	
	return fread(destination, size, count, wad->handle.file);
}

static wadfuncs_t WI_FILE_WADFUNCS = {
	wi_file_destroy,
	wi_file_commit_entries,
	wi_file_create_entry_at,
	wi_file_add_entry_at,
	wi_file_add_entry_data_at,
	wi_file_remove_entry_at,
	wi_file_get_data,
	wi_file_read_data,
};

// ===========================================================================
// WI_BUFFER
// ===========================================================================

// Implementation of wadfuncs_t.destroy(wad_t*)
static int wi_buffer_destroy(wad_t *wad)
{
	// Free buffer itself.
	WAD_FREE(wad->handle.buffer);
	wad->buffer_size = 0;
	wad->buffer_capacity = -1;
	return 0;
}

// Implementation of wadfuncs_t.commit_entries(wad_t*)
static int wi_buffer_commit_entries(wad_t *wad)
{
	// Do nothing. Nothing to write to.
	return 0;
}

// Implementation of wadfuncs_t.create_entry_at(wad_t*, const char*, int)
static wadentry_t* wi_buffer_create_entry_at(wad_t *wad, const char *name, int index)
{
	wadentry_t* entry;
	if (!(entry = WAD_AddEntryCommon(wad, name, 0, 0, index)))
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
		if (WAD_ExpandBuffer(wad, wad->buffer_capacity * 2))
			return 1;
	}
	
	return 0;
}

// Implementation of wadfuncs_t.add_entry_at(wad_t*, const char*, int, unsigned char*, size_t)
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
	if (!(entry = WAD_AddEntryCommon(wad, name, size, wad->buffer_size, index)))
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	
	wad->buffer_size += size;
	wad->header.entry_list_offset = wad->buffer_size + sizeof(wadheader_t);

	return entry;
}

// Implementation of wadfuncs_t.wi_map_add_entry_data_at(wad_t*, const char*, int, FILE*)
static wadentry_t* wi_buffer_add_entry_data_at(wad_t *wad, const char *name, int index, FILE *stream)
{
	int buf = 0;
	int amount = 0;
	unsigned char *dest = NULL;
	while (buf = fread(cbuf, 1, CBUF_LEN, stream))
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
	if (!(entry = WAD_AddEntryCommon(wad, name, amount, pos, index)))
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	
	return entry;
}

// Implementation of wadfuncs_t.remove_entry_at(wad_t*, int)
static int wi_buffer_remove_entry_at(wad_t *wad, int index)
{
	return WAD_RemoveEntryCommon(wad, index);
}

// Implementation of wadfuncs_t.get_data(wad_t*, wadentry_t*, unsigned char*)
static int wi_buffer_get_data(wad_t *wad, wadentry_t *entry, unsigned char *destination)
{
	unsigned char *dest = destination;
	unsigned char *src;
	int32_t len = entry->length;
	// must offset by header length. Entry content is offset by that much.
	if (len > 0)
	{
		src = wad->handle.buffer + entry->offset - sizeof(wadheader_t);
		memcpy(dest, src, len);
		return len;
	}
	
	return 0;
}

// Implementation of wadfuncs_t.read_data(wad_t*, wadentry_t*, void*, size_t, size_t)
static int wi_buffer_read_data(wad_t *wad, wadentry_t *entry, void *destination, size_t size, size_t count)
{
	int out = 0;
	unsigned char *dest = destination;
	unsigned char *src;
	int32_t len = entry->length;
	
	// must offset by header length. Entry content is offset by that much.
	if (len)
		src = wad->handle.buffer + entry->offset - sizeof(wadheader_t);
	else
		src = wad->handle.buffer;
	
	while (len >= size && count--)
	{
		memcpy(dest, src, size);
		dest += size;
		src += size;
		len -= size;
		out++;
	}
	return out;
}

static wadfuncs_t WI_BUFFER_WADFUNCS = {
	wi_buffer_destroy,
	wi_buffer_commit_entries,
	wi_buffer_create_entry_at,
	wi_buffer_add_entry_at,
	wi_buffer_add_entry_data_at,
	wi_buffer_remove_entry_at,
	wi_buffer_get_data,
	wi_buffer_read_data,
};

// ...........................................................................

static wadfuncs_t* WAD_funcs(wadimpl_t impl)
{
	switch (impl)
	{
		case WI_MAP: return &WI_MAP_WADFUNCS;
		case WI_FILE: return &WI_FILE_WADFUNCS;
		case WI_BUFFER: return &WI_BUFFER_WADFUNCS;
	}
	
	return NULL;
}

#define WI_FUNC(w,f) (WAD_funcs((w)->type))->f

// ===========================================================================
// Public Functions
// ===========================================================================

// ---------------------------------------------------------------
// wad_t* WAD_Open(char *filename)
// See wad.h
// ---------------------------------------------------------------
wad_t* WAD_Open(char *filename)
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

	out = WAD_Init();
	if (!out)
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	if (WAD_SetupOpenFileHandle(fp, &(out->header)))
	{
		waderrno = WADERROR_FILE_NOT_A_WAD;
		return NULL;
	}
	if (WAD_SetupBuildEntrylist(fp, out))
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}

	out->type = WI_FILE;
	out->handle.file = fp;	
	
	return out;
}

// ---------------------------------------------------------------
// wad_t* WAD_Create(char *filename)
// See wad.h
// ---------------------------------------------------------------
wad_t* WAD_Create(char *filename)
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

	out = WAD_Init();
	if (!out)
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	if (WAD_ExpandEntrylist(out, 8))
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
	if (WAD_SetupOpenFileHandle(fp, &(out->header)))
	{
		waderrno = WADERROR_FILE_NOT_A_WAD;
		return NULL;
	}

	return out;
}

// ---------------------------------------------------------------
// wad_t* WAD_OpenMap(char *filename)
// See wad.h
// ---------------------------------------------------------------
wad_t* WAD_OpenMap(char *filename)
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

	out = WAD_Init();
	if (!out)
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	if (WAD_SetupOpenFileHandle(fp, &(out->header)))
	{
		waderrno = WADERROR_FILE_NOT_A_WAD;
		return NULL;
	}
	if (WAD_SetupBuildEntrylist(fp, out))
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}

	fclose(fp);
	out->type = WI_MAP;

	return out;
}

// ---------------------------------------------------------------
// wad_t* WAD_OpenBuffer(char *filename)
// See wad.h
// ---------------------------------------------------------------
wad_t* WAD_OpenBuffer(char *filename)
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

	out = WAD_Init();
	if (!out)
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	if (WAD_SetupOpenFileHandle(fp, &(out->header)))
	{
		waderrno = WADERROR_FILE_NOT_A_WAD;
		return NULL;
	}
	if (WAD_SetupBuildEntrylist(fp, out))
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	if (WAD_SetupBuildBuffer(fp, out))
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}

	out->type = WI_BUFFER;

	return out;
}

// ---------------------------------------------------------------
// wad_t* WAD_CreateBuffer()
// See wad.h
// ---------------------------------------------------------------
wad_t* WAD_CreateBuffer()
{
	return WAD_CreateBufferInit(WADBUFFER_INITSIZE);
}

// ---------------------------------------------------------------
// wad_t* WAD_CreateBufferInit(int size)
// See wad.h
// ---------------------------------------------------------------
wad_t* WAD_CreateBufferInit(int size)
{
	wad_t *out;

	// Reset error state.
	waderrno = WADERROR_NO_ERROR;

	out = WAD_Init();
	if (!out)
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	if (WAD_ExpandEntrylist(out, 16))
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}
	if (WAD_ExpandBuffer(out, size))
	{
		waderrno = WADERROR_OUT_OF_MEMORY;
		return NULL;
	}

	out->type = WI_BUFFER;

	return out;
}

// ---------------------------------------------------------------
// int WAD_EntryCount(wad_t *wad)
// See wad.h
// ---------------------------------------------------------------
int WAD_EntryCount(wad_t *wad)
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
// waditerator_t* WAD_IteratorCreate(wad_t *wad, int start)
// See wad.h
// ---------------------------------------------------------------
waditerator_t* WAD_IteratorCreate(wad_t *wad, int start)
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
// void WAD_IteratorReset(waditerator_t *iter, int start)
// See wad.h
// ---------------------------------------------------------------
void WAD_IteratorReset(waditerator_t *iter, int start)
{
	iter->entry = NULL;
	iter->next = start;
	iter->count = iter->wad->header.entry_count;
}

// ---------------------------------------------------------------
// wadentry_t* WAD_IteratorNext(waditerator_t *iter)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* WAD_IteratorNext(waditerator_t *iter)
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
// void WAD_IteratorClose(waditerator_t *iter)
// See wad.h
// ---------------------------------------------------------------
void WAD_IteratorClose(waditerator_t *iter)
{
	WAD_FREE(iter);
}

// ---------------------------------------------------------------
// int WAD_CommitEntries(wad_t *wad)
// See wad.h
// ---------------------------------------------------------------
int WAD_CommitEntries(wad_t *wad)
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
// wadentry_t* WAD_GetEntry(wad_t *wad, int index)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* WAD_GetEntry(wad_t *wad, int index)
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
// wadentry_t* WAD_GetEntryByName(wad_t *wad, char *name)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* WAD_GetEntryByName(wad_t *wad, const char *name)
{
	return WAD_GetEntryByNameOffset(wad, name, 0);
}

// ---------------------------------------------------------------
// wadentry_t* WAD_GetEntryByNameOffset(wad_t *wad, char *name, int start)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* WAD_GetEntryByNameOffset(wad_t *wad, const char *name, int start)
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
// wadentry_t* WAD_GetEntryByNameNth(wad_t *wad, char *name, int nth)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* WAD_GetEntryByNameNth(wad_t *wad, const char *name, int nth)
{
	return WAD_GetEntryByNameOffsetNth(wad, name, 0, nth);
}

// ---------------------------------------------------------------
// wadentry_t* WAD_GetEntryByNameOffsetNth(wad_t *wad, char *name, int start, int nth)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* WAD_GetEntryByNameOffsetNth(wad_t *wad, const char *name, int start, int nth)
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
// wadentry_t* WAD_GetLastEntryByName(wad_t *wad, char *name)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* WAD_GetLastEntryByName(wad_t *wad, const char *name)
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
// int WAD_GetEntryCount(wad_t *wad, char *name)
// See wad.h
// ---------------------------------------------------------------
int WAD_GetEntryCount(wad_t *wad, const char *name)
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
// int WAD_GetEntryIndex(wad_t *wad, char *name)
// See wad.h
// ---------------------------------------------------------------
int WAD_GetEntryIndex(wad_t *wad, const char *name)
{
	return WAD_GetEntryIndexOffset(wad, name, 0);
}

// ---------------------------------------------------------------
// int WAD_GetEntryIndexOffset(wad_t *wad, char *name, int start)
// See wad.h
// ---------------------------------------------------------------
int WAD_GetEntryIndexOffset(wad_t *wad, const char *name, int start)
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
// WAD_GetEntryIndices(wad_t *wad, const char *name, int *out, int offset, int max)
// See wad.h
// ---------------------------------------------------------------
int WAD_GetEntryIndices(wad_t *wad, const char *name, int *out, int max)
{
	return WAD_GetEntryIndicesOffset(wad, name, 0, out, max);
}

// ---------------------------------------------------------------
// WAD_GetEntryIndicesOffset(wad_t *wad, const char *name, int start, int *out, int max)
// See wad.h
// ---------------------------------------------------------------
int WAD_GetEntryIndicesOffset(wad_t *wad, const char *name, int start, int *out, int max)
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
	while (i < max && (*out = WAD_GetEntryIndexOffset(wad, name, start)) >= 0)
	{
		start = (*out) + 1;
		out += 1;
		i++;
	}
	
	return i;
}

// ---------------------------------------------------------------
// int WAD_GetEntryLastIndex(wad_t *wad, char *name)
// See wad.h
// ---------------------------------------------------------------
int WAD_GetEntryLastIndex(wad_t *wad, const char *name)
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
// wadentry_t* WAD_CreateEntry(wad_t *wad, char *name)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* WAD_CreateEntry(wad_t *wad, const char *name)
{
	return WAD_CreateEntryAt(wad, name, wad->header.entry_count);
}

// ---------------------------------------------------------------
// wadentry_t* WAD_CreateEntryAt(wad_t *wad, char *name, int index)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* WAD_CreateEntryAt(wad_t *wad, const char *name, int index)
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
// wadentry_t* WAD_AddEntry(wad_t *wad, char *name, unsigned char *buffer, size_t buffer_size)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* WAD_AddEntry(wad_t *wad, const char *name, unsigned char *buffer, size_t buffer_size)
{
	return WAD_AddEntryAt(wad, name, wad->header.entry_count, buffer, buffer_size);
}

// ---------------------------------------------------------------
// wadentry_t* WAD_AddEntryAt(wad_t *wad, char *name, int index, unsigned char *buffer, size_t buffer_size)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* WAD_AddEntryAt(wad_t *wad, const char *name, int index, unsigned char *buffer, size_t buffer_size)
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
// wadentry_t* WAD_AddEntryData(wad_t *wad, const char *name, FILE *stream)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* WAD_AddEntryData(wad_t *wad, const char *name, FILE *stream)
{
	return WAD_AddEntryDataAt(wad, name, wad->header.entry_count, stream);
}

// ---------------------------------------------------------------
// wadentry_t* WAD_AddEntryDataAt(wad_t *wad, const char *name, int index, FILE *stream)
// See wad.h
// ---------------------------------------------------------------
wadentry_t* WAD_AddEntryDataAt(wad_t *wad, const char *name, int index, FILE *stream)
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
// int WAD_RemoveEntryAt(wad_t *wad, int index)
// See wad.h
// ---------------------------------------------------------------
int WAD_RemoveEntryAt(wad_t *wad, int index)
{
	// Reset error state.
	waderrno = WADERROR_NO_ERROR;
	errno = 0;

	if (wad == NULL)
	{
		waderrno = WADERROR_WAD_INVALID;
		return 1;
	}
	
	if ((WI_FUNC(wad, remove_entry_at))(wad, index))
	{
		// waderrno/errno set in call.
		return 1;
	}

	return 0;
}

// ---------------------------------------------------------------
// int WAD_GetEntryData(wad_t *wad, wadentry_t *entry, void *destination)
// See wad.h
// ---------------------------------------------------------------
int WAD_GetEntryData(wad_t *wad, wadentry_t *entry, unsigned char *destination)
{
	// Reset error state.
	waderrno = WADERROR_NO_ERROR;
	errno = 0;

	if (wad == NULL)
	{
		waderrno = WADERROR_WAD_INVALID;
		return 1;
	}
	
	if ((WI_FUNC(wad, get_data))(wad, entry, destination) < 0)
	{
		// waderrno/errno set in call.
		return -1;
	}

	return 0;
}

// ---------------------------------------------------------------
// int WAD_ReadEntryData(wad_t *wad, wadentry_t *entry, void *destination, size_t size, size_t count)
// See wad.h
// ---------------------------------------------------------------
int WAD_ReadEntryData(wad_t *wad, wadentry_t *entry, void *destination, size_t size, size_t count)
{
	// Reset error state.
	waderrno = WADERROR_NO_ERROR;
	errno = 0;

	if (wad == NULL)
	{
		waderrno = WADERROR_WAD_INVALID;
		return 1;
	}
	
	if ((WI_FUNC(wad, read_data))(wad, entry, destination, size, count) < 0)
	{
		// waderrno/errno set in call.
		return -1;
	}

	return 0;
}

// ---------------------------------------------------------------
// int WAD_Close(wad_t *wad)
// See wad.h
// ---------------------------------------------------------------
int WAD_Close(wad_t *wad)
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
	
	WAD_FreeAllocated(wad);
	
	return 0;
}
