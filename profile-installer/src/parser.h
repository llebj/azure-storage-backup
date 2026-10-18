// This parser is pretty fragile and is very specific to this file format.
// A more general INI-style parser could be implemented as follows.
//
// Define the following structs:
//
// struct section {
//	struct slice* header,
//	struct slice* tag,
//	struct attribute* attributes
// };
//
// struct attribute {
//	struct slice* key,
//	struct slice* value
//};
//
// The above structs are used to build up a data structure of an INI file, with
// each setion having a header ([Header]), a tag ([:tag]), and a collection of
// attributes (Key = Value). This structure can be used to access slices into the
// original file buffer, which can then be interpreted however the calling code
// may wish. The file structure parsing can remain in the INI parser, while the
// domain specific parsing and validation can be handled by the client code.
//
// This parser structure can be re-used to parse other INI-like files, such as config
// files for the program itself.

#ifndef PARSER
#define PARSER

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ini.h"
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
