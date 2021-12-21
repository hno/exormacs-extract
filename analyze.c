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
int debug = 0;

struct __attribute__((__packed__)) volume_id_block {
	char volume[4];
	uint16_t user_numer;
	uint32_t sat_block;
	uint16_t sat_length;
	uint32_t directory_block;
	uint32_t pdl;
	uint32_t os_start_block;
	uint16_t os_length;
	uint32_t os_execution_address;
	uint32_t os_load_address;
	uint32_t generation_data;
	char description[20];
	uint32_t initial_version;
	uint16_t checksum;
	char diag_pattern[64];
	uint32_t diag_directory;
	uint32_t dump_start_block;
	uint16_t dump_length;
	uint32_t slt_start_block;
	uint16_t slt_length;
	char reserved[104];
	char exormacs[8];
};

struct __attribute__((__packed__)) secondary_directory_block {
	uint32_t next;
	char reserved[12];
	struct __attribute__((__packed__)) secondary_directory_block_entry {
		uint16_t user_numer;
		char name[8];
		uint32_t block;
		uint8_t reserved[2];
	} entry[15];
};

struct __attribute__((__packed__)) primary_directory_block {
	uint32_t next;
	uint16_t user_number;
	char catalogue[8];
	char reserved[2];
	struct __attribute__((__packed__)) primary_directory_block_entry {
		char name[10];
		char reserved1[2];
		uint32_t start;
		uint32_t end;
		uint32_t eof;
		uint32_t eor;
		uint8_t write_acess_code;
		uint8_t read_access_code;
		uint8_t attributes;
		uint8_t last_block_size;
		uint16_t record_size;
		char reserved2[1];
		uint8_t key_size;
		uint8_t fab_size;
		uint8_t block_size;
		uint16_t date_created;
		uint16_t date_assigned;
		char reserved3[8];
	} entry[20];
};

enum {
	FILE_TYPE_CONTIGUOUS = 0,
	FILE_TYPE_SEQUENTIAL = 1,
	FILE_TYPE_ISAM = 2,
	FILE_TYPE_ISAMD = 3
};

void read_at(FILE *in, int start_block, char *buf, int size)
{
	fseek(in, start_block * 0x100, SEEK_SET);
	fread(buf, size, 1, in);
}

void hexdump(const void *_buf, int size)
{
	const unsigned char *buf = _buf;
	for (int i = 0; i < size; i++)
		printf("%s%02X", size > 0 ? " ": "", buf[i]);
}

void hexdata(const char *label, const void *buf, int size)
{
	if (label)
		printf("%s ", label);
	hexdump(buf, size);
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

void print_primary_directory_block_entry(const char *catalogue, struct primary_directory_block_entry *entry)
{
	printf("%-8s/%-8.8s.%-4.4s ", catalogue, &entry->name[0], &entry->name[8]);
	int type = entry->attributes & 0xf;
	switch(type) {
	case FILE_TYPE_CONTIGUOUS:
		printf(" start=%-4d size=%-4d", be32toh(entry->start), be32toh(entry->end)+1);
		break;
	case FILE_TYPE_SEQUENTIAL:
		printf(" sqeuential");
		int record_size = be16toh(entry->record_size);
		if (record_size)
			printf(" record_size=%d", record_size);
		else
			printf(" dynamic_record_size");
		if (verbose) {
			printf(" ");
			hexdump(&entry, sizeof(entry));
		}
		// TODO: Any additional fields of interest?
		break;
	case FILE_TYPE_ISAM:
	case FILE_TYPE_ISAMD:
		printf(" ISAM");
		if (type == FILE_TYPE_ISAMD)
			printf(" null non-unique");
		// TODO: Many more fields.
		// TODO: Dive down into FAB data?
		break;
	}
	printf("\n");
}

void print_secondary_directory_block_entry(struct secondary_directory_block_entry *entry)
{
	printf("%-8.8s/               block=%-4d\n", entry->name, be32toh(entry->block));
}

char *prepare_path(const char *catalogue, struct primary_directory_block_entry *entry)
{
	char filename[10];
	static char path[32];
	memcpy(filename, entry->name, 8);
	filename[8] = 0;
	trim_spaces(filename);
	// Create output folder structure
	mkdir(output_folder, ACCESSPERMS);
	snprintf(path, sizeof(path), "%s/%s", output_folder, catalogue);
	mkdir(path, ACCESSPERMS);
	snprintf(path, sizeof(path), "%s/%s/%s.%s", output_folder, catalogue, filename, entry->name+8);
	return path;
}

void save_file(FILE *in, const char *catalogue, struct primary_directory_block_entry *entry)
{
	char *path = prepare_path(catalogue, entry);
	FILE *out = fopen(path, "ab");
	uint32_t blocks = be32toh(entry->end)+1;
	for (uint32_t file_block = 0; file_block < blocks; file_block++) {
		char buf[0x100];
		read_at(in, be32toh(entry->start) + file_block, buf, sizeof(buf));
		fwrite(buf, sizeof(buf), 1, out);
	}
	fclose(out);
}

void read_primary_directory_block(FILE *in, char *catalogue, uint32_t start_block)
{
	char buf[0x400];
	read_at(in, start_block, buf, sizeof(buf));
	struct primary_directory_block *table = (void *)buf;
	if (debug)
		hexdata("pdp", buf, 0x100);
	if (verbose)
		printf("Catalogue: %.8s\n", table->catalogue);
	for (int i = 0; i < 20; i++) {
		struct primary_directory_block_entry *entry = &table->entry[i];
		if (debug)
			hexdata("primary_directory_block_entry", entry, sizeof(*entry));
		if (!entry->name[0])
			continue;
		print_primary_directory_block_entry(catalogue, entry);
		int type = entry->attributes & 0xf;
		if (output_folder) {
			switch(type) {
			case FILE_TYPE_CONTIGUOUS:
				save_file(in, catalogue, entry);
				break;
			case FILE_TYPE_SEQUENTIAL:
				fprintf(stderr, "ERROR: Saving of sequential files not implemented yet");
				break;
			case FILE_TYPE_ISAM:
			case FILE_TYPE_ISAMD:
				fprintf(stderr, "ERROR: Saving of ISAM files not implemented yet");
				break;
			}
		}
	}
	// TODO: Chain to next
}

void read_secondary_directory_block(FILE *in, uint32_t start_block)
{
	char buf[0x100];
	read_at(in, start_block, buf, sizeof(buf));
	struct secondary_directory_block *table = (void *)buf;
	if (debug)
		hexdata("sdp", buf, sizeof(buf));
	for (int i = 0; i < 15; i++) {
		struct secondary_directory_block_entry *entry = &table->entry[i];
		if (debug)
			hexdata("secondary_directory_block_entry", entry, sizeof(*entry));
		if (!entry->name[0])
			continue;
		print_secondary_directory_block_entry(entry);
		char set_name[9];
		memcpy(set_name, entry->name, sizeof(entry->name));
		set_name[8] = 0;
		trim_spaces(set_name);
		read_primary_directory_block(in, set_name, be32toh(entry->block));
	}
	// TODO: Chain to next
}

void print_volume_id(struct volume_id_block *vid)
{
	printf("Volume %.4s - %.20s\n", vid->volume, vid->description);
}

void read_volume_id(FILE *in)
{
	char buf[0x100];
	read_at(in, 0, buf, sizeof(buf));
	struct volume_id_block *vid = (void *)buf;
	print_volume_id(vid);
	read_secondary_directory_block(in, be32toh(vid->directory_block));
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

	while ((opt = getopt(argc, argv, "o:vd")) != -1) {
		switch(opt) {
		case 'o':
			output_folder = optarg;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'd':
			debug = 1;
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
		printf("Processing %s\n", file);
		FILE *in = fopen(file, "rb");
		if (!in) {
			perror("Open input file");
			exit(1);
		}
		read_volume_id(in);
		fclose(in);
	}
}
