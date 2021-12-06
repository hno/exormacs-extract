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
 *   mkdir output
 *
 *   for file in *.img; do
 *     ./analyze $file output
 *   done
 *
 ********************************************************************
 */

#include <stdio.h>
#include <endian.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

const char *output_folder = NULL;
int verbose = 0;

struct __attribute__((__packed__)) set_table {
	char unknown1[0xe];
	union {
		char raw[0x10];
		struct __attribute__((__packed__)) set_table_entry {
			uint32_t valid;
			char name[8];
			uint32_t block;
		} entry;
	} table[];
};

struct __attribute__((__packed__)) file_table {
	char unknown1[6];
	char setname[10];
	union {
		char raw[0x32];
		struct __attribute__((__packed__)) file_table_entry {
			char name[12];
			uint32_t block;
			uint32_t blocks;
		} entry;
	} table[];
};

void read_at(FILE *in, int start_block, char *buf, int size)
{
	fseek(in, start_block * 0x100, SEEK_SET);
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

void trim_spaces(char *name)
{
	for (int i = strlen(name) - 1; i >= 0; i--) {
		if (name[i] != ' ')
			break;
		name[i] = 0;
	}
}

void print_file_table_entry(const char *set, struct file_table_entry *entry)
{
	printf("%s/%-8.8s.%-4.4s ", set, &entry->name[0], &entry->name[8]);
	printf(" block=%-4d size=%-5d", be32toh(entry->block), be32toh(entry->blocks)+1);
	printf("\n");
}

void print_set_table_entry(struct set_table_entry *entry)
{
	printf("%-8.8s/ block=%-4d\n", entry->name, be32toh(entry->block));
}

void save_file(FILE *in, const char *set, struct file_table_entry *entry)
{
	char filename[10];
	char path[32];
	memcpy(filename, entry->name, 8);
	filename[8] = 0;
	trim_spaces(filename);
	// Create output folder structure
	mkdir(output_folder, ACCESSPERMS);
	snprintf(path, sizeof(path), "%s/%s", output_folder, set);
	mkdir(path, ACCESSPERMS);
	snprintf(path, sizeof(path), "%s/%s/%s.%s", output_folder, set, filename, entry->name+8);
	// Save file
	FILE *out = fopen(path, "ab");
	for (uint32_t file_block = 0; file_block < be32toh(entry->blocks)+1; file_block++) {
		char buf[0x100];
		read_at(in, (be32toh(entry->block) + file_block) * 0x100, buf, 0x100);
		fwrite(buf, 0x100, 1, out);
	}
	fclose(out);
}

void read_file_table(FILE *in, char *set, uint32_t start_block)
{
	char buf[0x400];
	read_at(in, start_block, buf, sizeof(buf));
	struct file_table *table = (void *)buf;
	//hexdump("block", buf, 0x100);
	if (verbose)
		hexdump("unknown", table->unknown1, sizeof(table->unknown1));
	printf("Set: %.8s\n", table->setname);
	for (int i = 0; i < 50 && table->table[i].entry.name[0]; i++) {
		if (verbose)
			hexdump("file_table_entry", table->table[i].raw, sizeof(table->table[i].raw));
		struct file_table_entry *entry = &table->table[i].entry;
		print_file_table_entry(set, entry);
		if (output_folder)
			save_file(in, set, entry);
	}
}

void read_set_table(FILE *in, uint32_t start_block)
{
	char buf[0x100];
	read_at(in, start_block, buf, sizeof(buf));
	struct set_table *table = (void *)buf;
	//hexdump("block", buf, 0x100);
	if (verbose)
		hexdump("unknown", table->unknown1, sizeof(table->unknown1));
	for (int i = 0; i < 50 && table->table[i].entry.valid; i++) {
		if (verbose)
			hexdump("set_table_entry", table->table[i].raw, sizeof(table->table[i].raw));
		struct set_table_entry *entry = &table->table[i].entry;
		print_set_table_entry(entry);
		char set_name[9];
		memcpy(set_name, entry->name, sizeof(entry->name));
		set_name[8] = 0;
		trim_spaces(set_name);
		read_file_table(in, set_name, be32toh(entry->block));
	}
}

const char *program_name = NULL;

void usage(int status)
{
	fprintf(stderr, "Usage: %s [-o output] [-v] input.img ...\n", program_name);
	fprintf(stderr, " -o output     Extracts all files into output folder\n");
	fprintf(stderr, "               the output folder should be empty on first file\n");
	fprintf(stderr, " -v            Verbose operation\n");
	exit(status);
}

int main(int argc, char **argv)
{
	int opt;
	program_name = argv[0];

	while ((opt = getopt(argc, argv, "o:v")) != -1) {
		switch(opt) {
		case 'o':
			output_folder = optarg;
			break;
		case 'v':
			verbose = 1;
			break;
		default: /* '?' */
			usage(0);
		}
	}
	if (optind >= argc)
		usage(1);

	for (int i = optind; i < argc; i++) {
		const char *file = argv[i];
		if (verbose)
			printf("Processing %s\n", file);
		FILE *in = fopen(file, "rb");
		if (!in) {
			perror("Open input file");
			exit(1);
		}
		read_set_table(in, 2);
		fclose(in);
	}
}
