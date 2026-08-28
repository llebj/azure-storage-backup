#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

bool count_profiles(size_t *count, char *buf, size_t buf_size);
bool parse_profiles(
		struct profile *profiles, size_t profiles_size,
		char* buf, size_t buf_size);

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

const char* profile_header = "Profile";

bool count_profiles(size_t *count, char *buf, size_t buf_size)
{
	bool success = true;
	*count = 0;
	for (size_t cursor = 0; cursor < buf_size; ++cursor) {
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
	Initial,
	Intermediate,
	ParsingHeader,
	ParsedHeader,
	ParsingKey,
	ParsingValue,
	Invalid
};

enum ParserState transition(enum ParserState current, char input);

bool parse_profiles(
		struct profile *profiles, size_t profiles_size,
		char *buf, size_t buf_size)
{
	bool result = true;
	enum ParserState current_state = Initial;
	size_t cursor = 0,
	       follow = 0;

	// the outer loop performs the state validation pass
	for ( ; cursor < buf_size; ++cursor) {
		enum ParserState new_state = transition(current_state, buf[cursor]);
		if (new_state == Invalid) {
			// The file format is invalid; we cannot parse the file.
			result = false;
			break;
		}
		if (current_state == new_state) {
			continue;
		}

		// We only want to consume the chars when the status changes.
		switch (current_state) {
		case Initial:
			break;
		case Intermediate:
			break;
		case ParsingHeader:
			break;
		case ParsedHeader:
			break;
		case ParsingKey:
			break;
		case ParsingValue:
			break;
		default:
			break;
		}
		current_state = new_state;
	}

	return result;
}

// Determine the new state based on the current state and the input
enum ParserState transition(enum ParserState current_state, char input)
{
	enum ParserState new_state = current_state;
	switch (current_state) {
	case Initial:
		switch (input) {
		case '[':
			new_state = ParsingHeader;
			break;
		case '\n':
			break;
		case ']':
		case '=':
		default:
			new_state = Invalid;
			break;
		}
		break;
	case Intermediate:
		switch (input) {
		case '[':
			new_state = ParsingHeader;
			break;
		case '\n':
			break;
		case ']':
		case '=':
			new_state = Invalid;
		default:
			new_state = ParsingKey;
			break;
		}
		break;
	case ParsingHeader:
		switch (input) {
		case ']':
			new_state = ParsedHeader;
			break;
		case '[':
		case '=':
		case '\n':
			new_state = Invalid;
			break;
		default:
			break;
		}
		break;
	case ParsedHeader:
		switch (input) {
		case '[':
		case ']':
		case '=':
			new_state = Invalid;
			break;
		case '\n':
			new_state = Intermediate;
			break;
		default:
			break;
		}
		break;
	case ParsingKey:
		switch (input) {
		case '[':
		case ']':
		case '\n':
			new_state = Invalid;
			break;
		case '=':
			new_state = ParsingValue;
			break;
		default:
			break;
		}
		break;
	case ParsingValue:
		switch (input) {
		case '[':
		case ']':
		case '=':
			new_state = Invalid;
			break;
		case '\n':
			new_state = Intermediate;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return new_state;
}
