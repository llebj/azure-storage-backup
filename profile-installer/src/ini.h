#ifndef INI
#define INI

#include <stddef.h>
#include <stdint.h>

#define INI_OK		0
#define INI_FAIL	1

enum ParserState {
	Initial = 1,
	Intermediate = 2,
	ParsingHeader = 4,
	ParsingTag = 8,
	ParsedHeader = 16,
	ParsingKey = 32,
	ParsingValue = 64,
	Invalid = 128
};

struct slice {
	char* start;
	size_t size;
};
struct ini {
	size_t size_section;
	struct section *sections;
};
struct section {
	struct slice header;
	struct slice tag;
	struct attribute* attributes;
 };
 struct attribute {
	struct slice key;
	struct slice value;
};

uint8_t parse_ini(struct slice *buf, struct ini *ini);

#endif
