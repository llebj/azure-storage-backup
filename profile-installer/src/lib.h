#ifndef MAIN
#define MAIN

#include <stddef.h>
#include <stdint.h>

#define SIZE_BUF	4

enum InstallerCode {
	PI_OK = 0,
	PI_FAIL = 1,
	PI_ALLOC_FAIL = 2,
	PI_READ_FAIL = 3,
	PI_PARSE_FAIL = 4
};
enum ProfileStatus {
	Active,
	Retired
};
enum TriggerType {
	PreInstall = 1,
	PostInstall = 2
};

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

uint64_t poly_hash(char *string);

#endif
