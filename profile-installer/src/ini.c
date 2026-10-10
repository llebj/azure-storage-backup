#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "ini.h"
#include "lib.h"

#define SIZE_BUF	4

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

struct state {
	enum ParserState current;
	enum ParserState previous;
};
struct scanner {
	struct slice source;
	size_t cursor;
	size_t mark;
};

uint8_t next_transition(char input, struct state *parser);
struct slice scanner_take(struct scanner *scanner);
struct slice trim(struct slice string);

uint8_t parse_ini(struct slice *buf, struct ini *ini)
{
	// TODO: check if inputs are NULL

	struct state state = {
		.current = Initial,
		.previous = Initial,
	};
	struct scanner scanner = {
		.source = *buf,
		.cursor = 0,
		.mark = 0
	};

	ini->sections = NULL;
	ini->size_section = 0;

	size_t section_i = 0,
	       attribute_i = 0;
	struct section *sections = NULL;
	struct attribute *attributes = NULL;

	// TODO: Handle re-allocation
	if ((sections = malloc(sizeof *sections * SIZE_BUF)) == NULL) {
		fprintf(stderr, "parse_ini: failed to allocate buffer for sections.\n");
		return PI_ALLOC_FAIL;
	}
	if ((attributes = malloc(sizeof *attributes * SIZE_BUF)) == NULL) {
		fprintf(stderr, "parse_ini: failed to allocate buffer for attributes.\n");
		return PI_ALLOC_FAIL;
	}

	for ( ; scanner.cursor < scanner.source.size; ++scanner.cursor) {
		uint8_t transition = next_transition(scanner.source.start[scanner.cursor], &state);
		if (transition == 0) {
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
			if (state.previous == Intermediate) {
				++section_i;
			}
			sections[section_i].attribs_offset = attribute_i;
			scanner_take(&scanner);
			break;
		case Intermediate | ParsingKey:
			// Do we need to do some validation here?
			scanner_take(&scanner);
			break;
		case ParsingHeader | ParsedHeader:
		case ParsingHeader | ParsingTag:
		{
			// Move start off of the starting char and start reading the token
			++scanner.mark;
			sections[section_i].header = scanner_take(&scanner);
			break;
		}
		case ParsingTag | ParsedHeader:
		{
			// Move start off of the starting char and start reading the token
			++scanner.mark;
			sections[section_i].tag = scanner_take(&scanner);
			break;
		}
		case ParsingKey | ParsingValue:
		{
			attributes[attribute_i].key = trim(scanner_take(&scanner));
			break;
		}
		case ParsingValue | Intermediate:
			// Move start off of '='
			++scanner.mark;

			attributes[attribute_i].value = trim(scanner_take(&scanner));
			sections[section_i].attribs_size = ++attribute_i - sections[section_i].attribs_offset;
			break;
		default:
			break;
		}
	}

	ini->sections = sections;
	ini->size_section = section_i + 1;
	ini->attributes = attributes;

	return INI_OK;
}

// Determine the next state based on the current state and the input
uint8_t next_transition(char input, struct state *parser)
{
	enum ParserState next = parser->current;

	switch (parser->current) {
	case Initial:
		switch (input) {
		case '[':
			next = ParsingHeader;
			break;
		case '\n':
			break;
		case ']':
		case '=':
		default:
			next = Invalid;
			break;
		}
		break;
	case Intermediate:
		switch (input) {
		case '[':
			next = ParsingHeader;
			break;
		case '\n':
			break;
		case ']':
		case '=':
			next = Invalid;
			break;
		default:
			next = ParsingKey;
			break;
		}
		break;
	case ParsingHeader:
		switch (input) {
		case ']':
			next = ParsedHeader;
			break;
		case ':':
			next = ParsingTag;
			break;
		case '[':
		case '=':
		case '\n':
			next = Invalid;
			break;
		default:
			break;
		}
		break;
	case ParsingTag:
		switch (input) {
		case ']':
			next = ParsedHeader;
			break;
		case ':':
		case '[':
		case '=':
		case '\n':
			next = Invalid;
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
			next = Invalid;
			break;
		case '\n':
			next = Intermediate;
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
			next = Invalid;
			break;
		case '=':
			next = ParsingValue;
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
			next = Invalid;
			break;
		case '\n':
			next = Intermediate;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	parser->previous = parser->current;
	parser->current = next;

	return parser->previous ^ parser->current;
}

struct slice scanner_take(struct scanner *scanner)
{
	struct slice token = {
		.start = scanner->source.start + scanner->mark,
		.size = scanner->cursor - scanner->mark
	};
	scanner->mark = scanner->cursor;

	return token;
}

struct slice trim(struct slice string)
{
	for ( ; string.size > 0 && isspace((unsigned char)string.start[0]); ++string.start, --string.size) {}
	for ( ; string.size > 0 && isspace((unsigned char)string.start[string.size -1]); --string.size) {}

	return string;
}
