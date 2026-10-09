#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "ini.h"
#include "lib.h"

#define SIZE_BUF	4

enum ParserState transition(enum ParserState current_state, char input);

uint8_t parse_ini(struct slice *buf, struct ini *ini)
{
	// TODO: check if inputs are NULL

	ini->sections = NULL;
	ini->size_section = 0;

	enum ParserState current_state = Initial;
	size_t cursor = 0,
	       follow = 0,
	       current_section = 0,
	       size_sections = 0;
	struct section *sections = NULL;

	// TODO: Handle re-allocating profiles if needed
	if ((sections = malloc(sizeof *sections * SIZE_BUF)) == NULL) {
		fprintf(stderr, "Failed to allocate buffer for profiles.\n");
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
			break;
		case ParsingHeader | ParsedHeader:
		{
			// Move follow off of '[' and start reading the name
			++follow;

			sections[current_section].header.start = buf->start + follow;
			sections[current_section].header.size = cursor - follow;

			follow = cursor;
			++size_sections;
			break;
		}
		default:
			break;
		}

		current_state = new_state;
	}

	ini->sections = sections;
	ini->size_section = size_sections;

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
		case '[':
		case '=':
		case '\n':
			new_state = Invalid;
			break;
		default:
			break;
		}
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
	default:
		break;
	}
	return new_state;
}
