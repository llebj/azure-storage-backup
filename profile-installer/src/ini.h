#ifndef INI
#define INI

#include <stddef.h>
#include <stdint.h>

#define INI_OK		0
#define INI_FAIL	1

struct slice {
	char* start;
	size_t size;
};
struct ini {
	size_t size_section;
	struct section *sections;
	struct attribute *attributes;
};
struct section {
	struct slice header;
	struct slice tag;
	size_t attribs_size;
	size_t attribs_offset;
 };
 struct attribute {
	struct slice key;
	struct slice value;
};

uint8_t parse_ini(struct slice *buf, struct ini *ini);

#endif
