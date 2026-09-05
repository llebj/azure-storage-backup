#ifndef PARSER
#define PARSER

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum ProfileType {
	Timer = 1,
	PreInstall = 2,
	PostInstall = 4
};
enum ParserState {
	Initial = 1,
	Intermediate = 2,
	ParsingHeader = 4,
	ParsedHeader = 8,
	ParsingKey = 16,
	ParsingValue = 32,
	Invalid = 64
};
enum CurrentFileKey {
	None,
	Source,
	Destination,
	Type
};

struct slice {
	char* start;
	size_t length;
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
enum ParserState transition(enum ParserState current, char input);

#endif
