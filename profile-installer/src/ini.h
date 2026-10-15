#ifndef INI
#define INI

#include "lib.h"
#include <stddef.h>
#include <stdint.h>

#define SIZE_BUF	4

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

enum InstallerCode parse_ini(struct slice *buf, struct ini *ini);

#endif
