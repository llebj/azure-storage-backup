#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <systemd/sd-id128.h>

#include "lib.h"
#include "parser.h"

#define APP_ID	SD_ID128_MAKE(e1,dc,79,91,d6,3a,45,36,09,0d,04,2a,c8,2a,50,91)

struct profile * map_profiles(struct parser_profile *parser_profiles, size_t count);

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <path>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	struct stat sb;
	if (stat(argv[1], &sb) == -1) {
		perror("stat");
		exit(EXIT_FAILURE);
	}
	if (!S_ISREG(sb.st_mode)) {
		fprintf(stderr, "File %s is not a regular file\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	int fd;
	if ((fd = open(argv[1], O_RDONLY)) == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	
	char *fb;
	if ((fb = malloc(sizeof *fb * sb.st_size)) == NULL) {
		fprintf(stderr, "Failed to allocate file buffer.\n");
		exit(EXIT_FAILURE);
	}

	if (read(fd, fb, sb.st_size) == -1) {
		perror("read");
		exit(EXIT_FAILURE);
	}

	size_t parser_profile_count = 0;
	struct parser_profile *parser_profiles = parse_profiles(&parser_profile_count, fb, sb.st_size);
	if (parser_profiles == NULL) {
		fprintf(stderr, "Failed to parse profiles.\n");
		exit(EXIT_FAILURE);
	}
	if (parser_profile_count == 0) {
		fprintf(stdout, "No profiles defined.\n");
		exit(EXIT_SUCCESS);
	}

	// how do we uniquely identify a profile?
	//	 using the fingerprint (hash of machine_id and source)
	// what happens if a profile changes?
	//	changing the following fields causes a version bump:
	//		- name
	//		- destination
	//		- trigger_type
	// bumped versions are inserted as new records
	//	previous versions get marked as 'retired'
	// what happens if a profile is deleted?
	//	it gets marked as 'retired'



	// compute fingerprints for all new profiles
	map_profiles(parser_profiles, parser_profile_count);

	// sort by fingerprints
	// retrieve all existing profiles sorted by fingerprint
	//
	// merge across profiles
	//	if DB key < parsed key
	//		deleted from DB
	//	else if DB key > parsed key
	//		insert new profile
	//	else
	//		insert new version of existing profile
}

struct profile * map_profiles(struct parser_profile *parser_profiles, size_t count)
{
	if (parser_profiles == NULL) {
		return NULL;
	}

	struct profile *profiles = NULL;
	if ((profiles = malloc(sizeof *profiles * count)) == NULL) {
		fprintf(stderr, "Failed to allocate buffer for profiles.\n");
		exit(EXIT_FAILURE);
	}

	size_t max_source = 0;
	for (size_t i = 0; i < count; ++i) {
		size_t current_len = strlen(parser_profiles[i].source);
		if (current_len > max_source) {
			max_source = current_len;
		}
	}

	sd_id128_t app_id = APP_ID;
	sd_id128_t raw_machine_id;
	if (sd_id128_get_machine_app_specific(app_id, &raw_machine_id) != 0) {
		fprintf(stderr, "Failed to generate machine ID.\n");
		return NULL;
	}
	char * machine_id = SD_ID128_TO_STRING(raw_machine_id);

	char * hash_source = NULL;
	// `SD_ID128_STRING_MAX` already includes for NUL terminator.
	size_t buf_len = max_source + SD_ID128_STRING_MAX;
	if ((hash_source = malloc(sizeof *hash_source * buf_len)) == NULL) {
		fprintf(stderr, "Failed to allocate hash source buffer.\n");
		return NULL;
	}

	for (size_t i = 0; i < SD_ID128_STRING_MAX; ++i) {
		hash_source[i] = machine_id[i];
	}
	for (size_t current = 0; current < count; ++current) {
		struct parser_profile current_profile = parser_profiles[current];
		size_t h_source_length = SD_ID128_STRING_MAX + strlen(current_profile.source);
		for (size_t i = SD_ID128_STRING_MAX - 1, j = 0; i < h_source_length; ++i, ++j) {
			hash_source[i] = current_profile.source[j];
		}
		uint64_t hash = poly_hash(hash_source);
	}
}
