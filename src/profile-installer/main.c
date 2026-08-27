#include <fcntl.h>
#include <stdbool.h>
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

bool count_profiles(size_t *count, char* buf, size_t buf_size);

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

	size_t profile_count = 0;
	if (!count_profiles(&profile_count, fb, sb.st_size)) {
		fprintf(stderr, "Profile configuration file is invalid.\n");
		exit(EXIT_FAILURE);
	}

	// allocate profile buffer
	struct profile *profiles;
	if ((profiles = malloc(sizeof *profiles * profile_count)) == NULL) {
		fprintf(stderr, "Failed to allocate profile buffer.\n");
		exit(EXIT_FAILURE);
	}

	// parse profiles
}

const char* profile_header = "Profile";

bool count_profiles(size_t *count, char* buf, size_t buf_size)
{
	size_t cursor = 0;
	bool success = true;
	*count = 0;
	for ( ; cursor < buf_size; ++cursor) {
		// Iterate until we find a section definition.
		if (buf[cursor] != '[') {
			continue;
		}

		size_t follow = cursor;
		// We have found a section definition, so now iterate until
		// we find the end of the definition header.
		for ( ; cursor < buf_size; ++cursor) {
			if (buf[cursor] != ']') {
				continue;
			}
			break;
		}
		if (cursor == buf_size) {
			// The file is invalid as there is no matching closing
			// bracket.
			success = false;
			break;
		}

		// At this point `follow` is pointing to '[' and `cursor` is
		// pointing to ']'. We start the comparison from the first character
		// following the `[`.
		if (strncmp(&buf[follow + 1], profile_header, strlen(profile_header)) == 0) {
			++*count;
		}
	}
	
	return success;
}

enum ParserState {
	Sentinel,
	ProfileHeader,
	Key,
	Value,
	Invalid
};

void parse_profiles(
		struct profile *profiles, size_t profiles_size,
		char* buf, size_t buf_size)
{
	enum ParserState state = Sentinel;
	size_t cursor = 0,
	       follow = 0;
	// move cursor to next special char
	// validate state
	// consume chars
}

