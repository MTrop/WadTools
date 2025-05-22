#include <stdio.h>
#include <errno.h>
#include "wad/wad.h"
#include "wad/waderrno.h"

extern int errno;
extern int waderrno;

#define INDICES_MAX 64
int indices[INDICES_MAX];

void print_entry(wadentry_t *entry)
{
	if (entry)
		printf("Entry: %s, len %d, ofs %d\n", entry->name, entry->length, entry->offset);
	else
		printf("No Entry\n");
}

void print_index(int i)
{
	printf("Index: %d\n", i);
}

void print_indices(int *array, int amount)
{
	int i = 0;
	printf("Indices: ");
	for (i = 0; i < amount; i++)
		printf("%d ", array[i]);
	printf("\n");
}

void print_wad(wad_t *wad)
{
	printf("Type %s\n", wad->header.type == WADTYPE_IWAD ? "IWAD" : "PWAD");
	printf("Content Size %d bytes\n", wad->header.entry_list_offset - sizeof(wadheader_t));
	printf("List start at %d\n", wad->header.entry_list_offset);
	
	waditerator_t iter;
	WAD_IteratorInit(&iter, wad, 0);
	wadentry_t *entry;
	int i = 0;
	printf("---- Name     Size     Offset\n");
	while ((entry = WAD_IteratorNext(&iter)))
		printf("%04d %-8.8s %-8d %-9d\n", i++, entry->name, entry->length, entry->offset);
	printf("Count %d\n", wad->header.entry_count);
	printf("Capacity %d\n", wad->entries_capacity);
}

void print_wad2(wad_t *wad)
{
	printf("Type %s\n", wad->header.type == WADTYPE_IWAD ? "IWAD" : "PWAD");
	printf("Content Size %d bytes\n", wad->header.entry_list_offset - sizeof(wadheader_t));
	printf("List start at %d\n", wad->header.entry_list_offset);
	
	wadentry_t *entry;
	int i = 0;
	printf("---- Name     Size     Offset\n");
	for (i = 0; i < wad->entries_capacity; i++)
	{
		entry = wad->entries[i];
		printf("%04d %08x %-8.8s %-8d %-9d\n", i, (unsigned int)entry, entry->name, entry->length, entry->offset);
	}
	printf("Count %d\n", wad->header.entry_count);
	printf("Capacity %d\n", wad->entries_capacity);
}


void print_error()
{
	if (!waderrno)
		return;
	
	if (waderrno == WADERROR_FILE_ERROR)
		printf("ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
	else
		printf("ERROR: %s\n", strwaderror(waderrno));
}

int main(int argc, char** argv)
{
	wad_t* wad = WAD_Create("TEST.wad");
	
	if (!wad)
	{
		if (waderrno == WADERROR_FILE_ERROR)
			printf("ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
		else
			printf("ERROR: %s\n", strwaderror(waderrno));
		return 1;
	}

	WAD_CreateEntry(wad, "LUMP00");
	WAD_CreateEntry(wad, "LUMP01");
	WAD_CreateEntry(wad, "LUMP02");
	WAD_CreateEntryAt(wad, "LUMP03", 1);
	
	char *junk = "This is some kind of message.";
	char *junk2 = "This is another type of message.";
	
	WAD_AddEntry(wad, "LUMP04", (unsigned char*)junk, strlen(junk));
	print_error();
	
	WAD_AddEntryAt(wad, "LUMP05", 2, (unsigned char*)junk2, strlen(junk2));
	print_error();

	FILE *f1 = fopen("gcc.txt", "rb");
	FILE *f2 = fopen(".\\src\\wad\\wad.c", "rb");
	
	WAD_AddEntryData(wad, "LUMP06", f1);
	print_error();
	WAD_AddEntryDataAt(wad, "LUMP07", 3, f2);
	print_error();
	print_wad(wad);

	WAD_RemoveEntryAt(wad, 3);
	print_error();
	print_wad(wad);

	WAD_RemoveEntryAt(wad, 7);
	print_error();
	print_wad(wad);
	
	fclose(f1);
	fclose(f2);

	int x[3] = {0, 3, 4};
	WAD_RemoveEntriesAt(wad, x, 3);
	print_error();
	print_wad(wad);

	WAD_RemoveEntryRange(wad, 1, 2);
	print_error();
	print_wad(wad);

	WAD_AddExplicitEntry(wad, "ADD1", 20, 12);
	print_error();
	print_wad(wad);

	WAD_AddExplicitEntryAt(wad, "ADD2", 2, 20, 32);
	print_error();
	print_wad(wad);

	WAD_AddMarkerEntryAt(wad, "F_START", 0);
	print_error();
	print_wad(wad);

	WAD_AddMarkerEntry(wad, "F_END");
	print_error();
	print_wad(wad);

	WAD_SwapEntry(wad, 0, 1);
	print_error();
	print_wad(wad);

	WAD_ShiftEntry(wad, 2, 1);
	print_error();
	print_wad(wad);

	WAD_ShiftEntry(wad, 1, 2);
	print_error();
	print_wad(wad);

	WAD_ShiftEntries(wad, 1, 3, 3);
	print_error();
	print_wad(wad);

	WAD_ShiftEntries(wad, 4, 2, 0);
	print_error();
	print_wad(wad);

	WAD_SwapEntry(wad, 5, 4);
	print_error();
	print_wad(wad);

	printf("DONE\n");

	WAD_Close(wad);
	return 0;
}
