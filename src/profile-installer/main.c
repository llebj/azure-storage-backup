#include <cstdint>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "parser.h"

enum ProfileStatus {
	Active,
	Retired
};

struct db_profile {
	uint32_t id;
	uint64_t fingerprint;
	uint32_t version;
	unsigned char *name;
	unsigned char *source;
	unsigned char *destination;
	uint8_t trigger_type;
	enum ProfileStatus status;
};

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

	size_t profile_count = 0;
	struct profile *profiles = parse_profiles(&profile_count, fb, sb.st_size);
	if (profiles == NULL) {
		fprintf(stderr, "Failed to parse profiles.\n");
		exit(EXIT_FAILURE);
	}
	if (profile_count == 0) {
		fprintf(stdout, "No profiles defined.\n");
		exit(EXIT_SUCCESS);
	}

	// commit profiles
	//	how do we uniquely identify a profile?
	//		using the fingerprint (hash of machine_id and source)
	//	what happens if a profile changes?
	//		changing the following fields causes a version bump:
	//			- name
	//			- destination
	//			- trigger_type
	//		bumped versions are inserted as new records
	//		previous versions get marked as 'retired'
	//	what happens if a profile is deleted?
	//		it gets marked as 'retired'
}

