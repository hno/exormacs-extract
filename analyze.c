/* Simple program to dump files from EXORMACS backup sets
 *
 * Copyright 2021 Henrik Nordström <henrik@henriknordstrom.net>
 *
 *   Permission is hereby granted, free of charge, to any person
 *   obtaining a copy of this software and associated documentation
 *   files (the "Software"), to deal in the Software without
 *   restriction, including without limitation the rights to use,
 *   copy, modify, merge, publish, distribute, sublicense, and/or
 *   sell copies of the Software, and to permit persons to whom the
 *   Software is furnished to do so, subject to the following conditions:
 *
 *   - The above copyright notice and this permission notice shall be
 *     included in all copies or substantial portions of the Software.
 *
 *   - THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *     EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *     OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *     NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *     HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *     WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *     FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *     OTHER DEALINGS IN THE SOFTWARE.
 *
 ********************************************************************
 *
 * This application analyzes an image file and extracts any files
 * found with. The output folder should be empty before analyzing
 * a new set of images as data is appended to each file to handle
 * files crossing several image files
 *
 *   mkdir ourput
 *   for file in img/*.img; do
 *     ./analyze $file output
 *   done
 *
 */

#include <stdio.h>
#include <endian.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

struct __attribute__((__packed__)) dirblock {
	char unknown1[6];
	char label[10];
	union {
		char raw[0x342-0x310];
		struct __attribute__((__packed__)) direntry {
			char name[12];
			uint32_t first_block;
			uint32_t blocks;
		} entry;
	} dir[];
};

void read_at(FILE *in, int offset, char *buf, int size)
{
	fseek(in, offset, SEEK_SET);
	fread(buf, size, 1, in);
}

void hexdump(const char *label, const char *buf, int size)
{
	if (label)
		printf("%s: ", label);
	for (int i = 0; i < size; i++)
		printf("%s%02X", size > 0 ? " ": "", (unsigned char)buf[i]);
	printf("\n");
}

void print_dir_entry(struct direntry *entry)
{
	printf("%-8.8s.%-4.4s ", &entry->name[0], &entry->name[8]);
	printf(" first_block=%-4d size=%-5d", be32toh(entry->first_block), be32toh(entry->blocks));
	printf("\n");
}

void save_file(FILE *in, const char *set, struct direntry *entry)
{
	char filename[10];
	char path[32];
	memcpy(filename, entry->name, 8);
	filename[8] = 0;
	for (int i = 7; i > 0; i--) {
		if (filename[i] != ' ')
			break;
		filename[i] = 0;
	}
	snprintf(path, sizeof(path), "%s/%s.%s", set, filename, entry->name+8);
	FILE *out = fopen(path, "ab");
	for (uint32_t file_block = 0; file_block < be32toh(entry->blocks); file_block++) {
		char buf[0x100];
		read_at(in, (be32toh(entry->first_block) + file_block) * 0x100, buf, 0x100);
		fwrite(buf, 0x100, 1, out);
	}
	fclose(out);
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		fprintf(stderr, "Usage: %s input.img outputfolder\n", argv[0]);
		exit(1);
	}
	char buf[0x400];
	FILE *in = fopen(argv[1], "rb");
	read_at(in, 0x300, buf, sizeof(buf));
	struct dirblock *dp = (void *)buf;
	//hexdump("block", buf, 0x100);
	hexdump("unknown", dp->unknown1, sizeof(dp->unknown1));
	printf("Label: %.16s\n", dp->label);
	for (int i = 0; i < 50 && dp->dir[i].entry.name[0]; i++) {
		hexdump("entry", dp->dir[i].raw, sizeof(dp->dir[i].raw));
		print_dir_entry(&dp->dir[i].entry);
		save_file(in, argv[2], &dp->dir[i].entry);
	}
}
