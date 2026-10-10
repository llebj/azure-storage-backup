#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "ini.h"
#include "lib.h"

#define SIZE_BUF	4

enum ParserState transition(enum ParserState current_state, char input);
struct slice trim(struct slice string);

uint8_t parse_ini(struct slice *buf, struct ini *ini)
{
	// TODO: check if inputs are NULL

	ini->sections = NULL;
	ini->size_section = 0;

	enum ParserState current_state = Initial;
	size_t cursor = 0,
	       follow = 0;

	struct section *sections = NULL;
	size_t current_section = 0,
	       sections_size = 0;

	struct attribute *attributes = NULL;
	size_t current_attribute = 0;

	// TODO: Handle re-allocation
	if ((sections = malloc(sizeof *sections * SIZE_BUF)) == NULL) {
		fprintf(stderr, "parse_ini: failed to allocate buffer for sections.\n");
		return PI_ALLOC_FAIL;
	}
	if ((attributes = malloc(sizeof *attributes * SIZE_BUF)) == NULL) {
		fprintf(stderr, "parse_ini: failed to allocate buffer for attributes.\n");
		return PI_ALLOC_FAIL;
	}

	for ( ; cursor < buf->size; ++cursor) {
		enum ParserState new_state = transition(current_state, buf->start[cursor]);
		uint8_t transition = current_state | new_state;

		if (current_state == new_state) {
			continue;
		}

		// The action to take can be determined purely on the transition.
		// Each case represents a trasition arranged as `current_state | new_state`.
		// WARN: OR'd states works in this case because there are no bi-directional
		//       edges in the state-transition graph. If that property ever changes
		//       then this will have to be revisited.
		switch (transition) {
		case Initial | ParsingHeader:
		case Intermediate | ParsingHeader:
			if (current_state == Intermediate) {
				++current_section;
			}
			sections[current_section].attribs_offset = current_attribute;
			follow = cursor;
			break;
		case Intermediate | ParsingKey:
			// Do we need to do some validation here?
			follow = cursor;
			break;
		case ParsingHeader | ParsedHeader:
		case ParsingHeader | ParsingTag:
		{
			// Move follow off of the starting char and start reading the token
			++follow;

			sections[current_section].header.start = buf->start + follow;
			sections[current_section].header.size = cursor - follow;

			follow = cursor;
			++sections_size;
			break;
		}
		case ParsingTag | ParsedHeader:
		{
			// Move follow off of the starting char and start reading the token
			++follow;

			sections[current_section].tag.start = buf->start + follow;
			sections[current_section].tag.size = cursor - follow;

			follow = cursor;
			break;
		}
		case ParsingKey | ParsingValue:
		{
			struct slice token = {
				.start = buf->start + follow,
				.size = cursor - follow
			};
			token = trim(token);

			attributes[current_attribute].key.start = token.start;
			attributes[current_attribute].key.size = token.size;

			follow = cursor;
			break;
		}
		case ParsingValue | Intermediate:
			// Move follow off of '='
			++follow;

			struct slice token = {
				.start = buf->start + follow,
				.size = cursor - follow
			};
			token = trim(token);

			attributes[current_attribute].value.start = token.start;
			attributes[current_attribute].value.size = token.size;
			sections[current_section].attribs_size = ++current_attribute - sections[current_section].attribs_offset;

			follow = cursor;
			break;
		default:
			break;
		}

		current_state = new_state;
	}

	ini->sections = sections;
	ini->size_section = sections_size;
	ini->attributes = attributes;

	return INI_OK;
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
			break;
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
		case ':':
			new_state = ParsingTag;
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
	case ParsingTag:
		switch (input) {
		case ']':
			new_state = ParsedHeader;
			break;
		case ':':
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
		.size = 0
	};
	if (string.size == 0) {
		return result;
	}

	size_t i = 0,
	       j = string.size - 1;
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
		result.size = j - i + 1;
	}
	return result;
}
