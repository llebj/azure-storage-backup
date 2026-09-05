#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "parser.h"

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
	if (!count_profiles(&profile_count, fb, sb.st_size)) {
		fprintf(stderr, "Profile configuration file is invalid.\n");
		exit(EXIT_FAILURE);
	}

	struct profile *profiles;
	if ((profiles = malloc(sizeof *profiles * profile_count)) == NULL) {
		fprintf(stderr, "Failed to allocate profile buffer.\n");
		exit(EXIT_FAILURE);
	}

	parse_profiles(profiles, profile_count, fb, sb.st_size);
}

