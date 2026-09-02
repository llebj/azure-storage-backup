#include <ctype.h>
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
	Initial = 1,
	Intermediate = 2,
	ParsingHeader = 4,
	ParsedHeader = 8,
	ParsingKey = 16,
	ParsingValue = 32,
	Invalid = 64
};

struct slice {
	char* start;
	size_t length;
};

enum ParserState transition(enum ParserState current, char input);
struct slice trim(struct slice string);

bool parse_profiles(
		struct profile *profiles, size_t profiles_size,
		char *buf, size_t buf_size)
{
	enum ParserState current_state = Initial;
	size_t cursor = 0,
	       follow = 0;

	// TODO: Handle iterating through the profiles.

	// the outer loop performs the state validation pass
	for ( ; cursor < buf_size; ++cursor) {
		enum ParserState new_state = transition(current_state, buf[cursor]);
		if (new_state == Invalid) {
			current_state = Invalid;
			break;
		}

		if (current_state == new_state) {
			continue;
		}

		uint8_t transition = current_state | new_state;
		// The action to take can be determined purely on the transition.
		// Each case represents a trasition arranged as `current_state | new_state`.
		// WARN: OR'd states works in this case because there are no bi-directional
		//       edges in the state-transition graph. If that property ever changes
		//       then this will have to be revisited.
		switch (transition) {
		case Initial | ParsingHeader:
			// Skip contents
			// TODO: Implement
			follow = cursor;
			break;
		case Intermediate | ParsingHeader:
			break;
		case Intermediate | ParsingKey:
			break;
		case ParsingHeader | ParsedHeader:
		{
			// Move follow off of '[' and start reading the name
			++follow;
			// Try to establish the span containing the header label
			size_t start = follow;
			for ( ; follow < cursor; ++follow) {
				if (buf[follow] == ':') {
					break;
				}
				continue;
			}

			// Validate the header label to ensure that it is a profile
			bool no_colon = follow == cursor;
			bool incorrect_length = (follow - start) != strlen(profile_header);
			bool incorrect_content = strncmp(
						&buf[start],
						profile_header,
						strlen(profile_header)) != 0;
			if (no_colon || incorrect_length || incorrect_content) {
				current_state = Invalid;
				break;
			}

			// Move follow off of ':' and start reading the name
			++follow;
			// Validate that the profile name has positive length before
			// allocating any memory.
			if (follow >= cursor) {
				current_state = Invalid;
				break;
			}

			struct slice name = {
				.start = &buf[follow],
				.length = cursor - follow
			};
			name = trim(name);
			if (name.length == 0) {
				current_state = Invalid;
				break;
			}

			char *name_buf;
			if ((name_buf = malloc(sizeof *name_buf * (name.length + 1))) == NULL) {
				fprintf(stderr, "Failed to allocate buffer for profile name.\n");
				current_state = Invalid;
				break;
			}

			size_t i = 0;
			for ( ; i < name.length; ++i) {
				name_buf[i] = name.start[i];
			}
			name_buf[i] = '\0';

			profiles->name = name_buf;
			follow = cursor;
			break;
		}
		case ParsedHeader | Intermediate:
		{
			// Move follow off of ']'
			++follow;
			struct slice name = {
				.start = &buf[follow],
				.length = cursor - follow
			};
			name = trim(name);
			if (name.length != 0) {
				current_state = Invalid;
			}
			break;
		}
		case ParsingKey | ParsingValue:
			// extract key
			break;
		case ParsingValue | Intermediate:
			// extract value
			break;
		default:
			// Every other transition results in `Invalid`.
			break;
		}

		if (current_state == Invalid) {
			break;
		}

		current_state = new_state;

	}

	// TODO: Replace with IS_VALID() macro
	return current_state != Invalid;
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

struct slice trim(struct slice string)
{
	struct slice result = {
		.start = NULL,
		.length = 0
	};
	if (string.length == 0) {
		return result;
	}

	size_t i = 0,
	       j = string.length - 1;
	for (; i <= j; ) {
		// We exhaustively trim from the start first instead of trimming
		// from the start and end concurrently to prevent underflow of `j`.
		if (isspace(string.start[i])) {
			++i;
		}
		else if (isspace(string.start[j])) {
			--j;
		}
		else {
			// Neither `i`, nor `j` were moved, therefore we have
			// finished trimming.
			break;
		}
	}
	if (i <= j) {
		result.start = &string.start[i];
		result.length = j - i + 1;
	}
	return result;
}
