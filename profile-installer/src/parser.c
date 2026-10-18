#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "lib.h"
#include "parser.h"

enum CurrentFileKey get_key(struct slice string);
bool parse_type(struct slice string, uint8_t *type);
char * string_from(struct slice slice);

enum InstallerCode parse_profiles(
		struct ini *ini,
		struct file_profile *profiles)
{
	if (ini == NULL || profiles == NULL) {
		return PI_FAIL;
	}

	for (size_t s_i = 0; s_i < ini->size_section; ++s_i) {
		struct section current_section = ini->sections[s_i];
		if (strncmp("Profile", current_section.header.start, current_section.header.size) != 0) {
			return PI_PARSE_FAIL;
		}

		profiles[s_i] = (struct file_profile) {0};
		profiles[s_i].name = string_from(current_section.tag);
		for (size_t a_i = 0; a_i < current_section.attribs_size; ++a_i) {
			struct attribute current_attribute = ini->attributes[current_section.attribs_offset + a_i];
			enum CurrentFileKey key = get_key(current_attribute.key);
			switch (key) {
				case Source:
				{
					profiles[s_i].source = string_from(current_attribute.value);
					break;
				}
				case Type:
				{
					uint8_t type = 0;
					profiles[s_i].type = parse_type(current_attribute.value, &type) ? 
						type : 0;
					break;
				}
				default:
				{
					break;
				}
			}
		}

		if (profiles[s_i].name == NULL || profiles[s_i].source == NULL || profiles[s_i].type == 0) {
			return PI_PARSE_FAIL;
		}
	}

	return PI_OK;
}

bool parse_type(struct slice string, uint8_t *type)
{
	uint8_t value = 0;
	bool result = true;
	// `head` is exclusive when used to build the slice
	// so we need to iterate up to the token size.
	for (size_t head = 0, tail = 0; head <= string.size; ++head) {
		// WARN: Check size before reading from string.
		if (head < string.size && string.start[head] != ',') {
			continue;
		}

		struct slice type_name = {
			.start = string.start + tail,
			.size = head - tail
		};
		if (type_name.size == 0) {
			result = false;
			break;
		}

		if (strncmp(type_name.start, "preinstall", type_name.size) == 0) {
			value |= PreInstall;
		}
		else if (strncmp(type_name.start, "postinstall", type_name.size) == 0) {
			value |= PostInstall;
		}
		else {
			result = false;
			break;
		}

		// `i` is currently pointing at ',' so move to the next char
		tail = head + 1;
	}
	if (result) {
		*type = value;
	}

	return result;
}

enum CurrentFileKey get_key(struct slice string)
{
	enum CurrentFileKey result = None;
	if (strncmp(string.start, "Source", string.size) == 0) {
		result = Source;
	}
	else if (strncmp(string.start, "Type", string.size) == 0) {
		result = Type;
	}

	return result;
}

char * string_from(struct slice slice)
{
	char * result = NULL;
	if (slice.size == 0 || (result = malloc(slice.size + 1)) == NULL) {
		return NULL;
	}
	memcpy(result, slice.start, slice.size);
	result[slice.size] = '\0';

	return result;
}

