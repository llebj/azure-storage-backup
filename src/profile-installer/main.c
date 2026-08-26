#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

enum ProfileType {
	Timer = 1,
	PreInstall = 2,
	PostInstall = 4
};

struct profile {
	char* name;
	char* source;
	char* destination;
	uint8_t type;
};

int8_t count_profiles(char* buf, size_t size);

int main(int argc, char** argv)
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
	uint8_t profile_count = count_profiles(fb, sb.st_size);

	// allocate profile buffer
	struct profile *profiles;
	if ((profiles = malloc(sizeof *profiles * profile_count)) == NULL) {
		fprintf(stderr, "Failed to allocate profile buffer.\n");
		exit(EXIT_FAILURE);
	}

	// parse profiles
}

const char* profile_header = "profile";

int8_t count_profiles(char* buf, size_t size)
{
	uint32_t start = 0,
		 cursor = 0;
	int8_t count = 0;
	for ( ; cursor < size || buf[cursor] != EOF; ++cursor, ++start) {
		// Iterate until we find a section definition.
		if (buf[cursor] != '[') {
			continue;
		}
		// We have found a section definition, so now iterate until
		// we find the end of the definition header.
		for ( ; cursor < size || buf[cursor] != EOF; ++cursor) {
			if (buf[cursor] != ']') {
				continue;
			}
			break;
		}
		if (cursor == size || buf[cursor] == EOF) {
			// The file is invalid as there is no matching closing
			// bracket.
			count = -1;
			break;
		}

		// At this point `start` is pointing to '[' and `cursor` is
		// pointing to ']'.
		int is_equal = strncasecmp(&buf[start + 1], profile_header, strlen(profile_header)) != 0;
		if (is_equal == 0) {
			// The section is not a profile definition.
			++count;
		}
		start = cursor;
	}
	
	return count;
}

