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
	printf("Content Size %d bytes\n", wad->header.entry_list_offset - WADHEADER_LEN);
	printf("List start at %d\n", wad->header.entry_list_offset);
	
	waditerator_t *iter = wad_iterator_create(wad, 0);
	wadentry_t *entry;
	int i = 0;
	printf("---- Name     Size     Offset\n");
	while (entry = wad_iterator_next(iter))
		printf("%04d %-8s %-8d %-9d\n", i++, entry->name, entry->length, entry->offset);
	printf("Count %d\n", wad->header.entry_count);
	wad_iterator_close(iter);
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
	wad_t* wad = wad_create("TEST.wad");
//	wad_t* wad = wad_create_buffer();
	
	if (!wad)
	{
		if (waderrno == WADERROR_FILE_ERROR)
			printf("ERROR: %s %s\n", strwaderror(waderrno), strerror(errno));
		else
			printf("ERROR: %s\n", strwaderror(waderrno));
		return 1;
	}

	wad_create_entry(wad, "LUMP00");
	wad_create_entry(wad, "LUMP01");
	wad_create_entry(wad, "LUMP02");
	wad_create_entry_at(wad, "LUMP03", 1);
	
	unsigned char *junk = "This is some kind of message.";
	unsigned char *junk2 = "This is another type of message.";
	
	wad_add_entry(wad, "LUMP04", junk, strlen(junk));
	print_error();
	
	wad_add_entry_at(wad, "LUMP05", 2, junk2, strlen(junk2));
	print_error();

	FILE *f1 = fopen("gcc.txt", "rb");
	FILE *f2 = fopen(".\\src\\wad\\wad.c", "rb");
	
	wad_add_entry_data(wad, "LUMP06", f1);
	print_error();
	wad_add_entry_data_at(wad, "LUMP07", 3, f2);
	print_error();
	
	fclose(f1);
	fclose(f2);

	print_wad(wad);
	
	wad_close(wad);
	return 0;
}
