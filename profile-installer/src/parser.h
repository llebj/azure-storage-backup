#ifndef PARSER
#define PARSER

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lib.h"

enum CurrentFileKey {
	None,
	Source,
	Type
};

struct file_profile {
	char* name;
	char* source;
	uint8_t type;
};

enum InstallerCode parse_profiles(
		struct ini *ini,
		struct file_profile *profiles);

#endif
