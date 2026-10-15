#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "ini.h"
#include "lib.h"

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
struct position {
	size_t line;
	size_t character;
};
struct token {
	struct slice value;
	struct position position;
};
struct scanner {
	struct slice source;
	size_t cursor;
	size_t mark;
	struct position current_position;
	struct token last_token;
};

uint8_t next_transition(char input, struct state *parser);
void scanner_step(struct scanner *scanner);
struct slice scanner_take(struct scanner *scanner);
struct slice trim(struct slice string);
void * ensure_space(size_t i, size_t elem_size, void *p, size_t *size);

enum InstallerCode parse_ini(struct slice *buf, struct ini *ini)
{
	// TODO: check if inputs are NULL

	enum InstallerCode result = PI_OK;
	struct state state = {
		.current = Initial,
		.previous = Initial,
	};
	struct scanner scanner = {
		.source = *buf,
		.cursor = 0,
		.mark = 0,
		.current_position = (struct position) {
			.line = 1,
			.character = 1
		},
		.last_token = (struct token) {
			.value = {0},
			.position = (struct position) {
				.line = 1,
				.character = 1
			}
		}
	};
	bool invalid_token = false;

	ini->size_section = 0;
	ini->sections = NULL;
	ini->attributes = NULL;

	size_t section_i = 0,
	       sections_size = SIZE_BUF,
	       attribute_i = 0,
	       attributes_size = SIZE_BUF;
	struct section *sections = NULL;
	struct attribute *attributes = NULL;

	if ((sections = malloc(sizeof *sections * SIZE_BUF)) == NULL) {
		fprintf(stderr, "parse_ini: failed to allocate buffer for sections.\n");
		return PI_ALLOC_FAIL;
	}
	if ((attributes = malloc(sizeof *attributes * SIZE_BUF)) == NULL) {
		fprintf(stderr, "parse_ini: failed to allocate buffer for attributes.\n");
		return PI_ALLOC_FAIL;
	}

	for ( ; scanner.cursor < scanner.source.size; scanner_step(&scanner)) {
		uint8_t transition = next_transition(scanner.source.start[scanner.cursor], &state);
		if (state.current == Invalid) {
			break;
		}
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
				struct section *tmp = ensure_space(section_i, sizeof *sections, sections, &sections_size);
				if (tmp == NULL) {
					result = PI_ALLOC_FAIL;
					break;
				}
				sections = tmp;
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
			sections[section_i].header = trim(scanner_take(&scanner));
			if (sections[section_i].header.size == 0) {
				invalid_token = true;
				break;
			}
			break;
		}
		case ParsingTag | ParsedHeader:
		{
			// Move start off of the starting char and start reading the token
			++scanner.mark;
			sections[section_i].tag = trim(scanner_take(&scanner));
			if (sections[section_i].tag.size == 0) {
				invalid_token = true;
				break;
			}
			break;
		}
		case ParsingKey | ParsingValue:
		{
			struct attribute *tmp = ensure_space(attribute_i, sizeof *attributes, attributes, &attributes_size);
			if (tmp == NULL) {
				result = PI_ALLOC_FAIL;
				break;
			}
			attributes = tmp;
			attributes[attribute_i].key = trim(scanner_take(&scanner));
			if (attributes[attribute_i].key.size == 0) {
				invalid_token = true;
				break;
			}
			break;
		}
		case ParsingValue | Intermediate:
			// Move start off of '='
			++scanner.mark;

			attributes[attribute_i].value = trim(scanner_take(&scanner));
			if (attributes[attribute_i].value.size == 0) {
				invalid_token = true;
				break;
			}
			sections[section_i].attribs_size = ++attribute_i - sections[section_i].attribs_offset;
			break;
		default:
			break;
		}

		if (invalid_token) {
			// TODO: Store this as a retrievable messages to avoid polluting
			//       output during testing.
			fprintf(stderr, "parser_ini: token starting at %ld:%ld is invalid.\n",
					scanner.last_token.position.line, scanner.last_token.position.character);
			break;
		}
		if (result != PI_OK) {
			fprintf(stderr, "parser_ini: failed to parse file: %d\n", result);
			break;
		}
	}

	// Requiring `Intermediate` is essentially requiring the file to end with `\n`.
	// This is not ideal, but it simplifies the implementation as we don't have to
	// process the final token when we exit the loop if this condition wasn't imposed.
	if (state.current != Intermediate || invalid_token) {
		return PI_PARSE_FAIL;
	}
	else if (result != PI_OK) {
		return result;
	}

	ini->size_section = section_i + 1;
	ini->sections = sections;
	ini->attributes = attributes;

	return PI_OK;
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

void scanner_step(struct scanner *scanner)
{
	bool eol = scanner->source.start[scanner->cursor++] == '\n';
	if (eol) {
		++scanner->current_position.line;
		scanner->current_position.character = 1;
	}
	else {
		++scanner->current_position.character;
	}
}

struct slice scanner_take(struct scanner *scanner)
{
	struct slice token = {
		.start = scanner->source.start + scanner->mark,
		.size = scanner->cursor - scanner->mark
	};

	scanner->last_token = (struct token) {
		.value = token,
		.position = (struct position) {
			.line = scanner->current_position.line,
			// This breaks when the previous token is on another line
			// TODO: Fix broken last token calc
			.character = scanner->current_position.character - (scanner->cursor - scanner->mark)
		}
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

void * ensure_space(size_t i, size_t elem_size, void *p, size_t *current_size)
{
	size_t new_size = *current_size;
	for ( ; i >= new_size; ) {
		new_size = new_size > 0 ? new_size * 2 : SIZE_BUF;
	}
	if (new_size == *current_size) {
		return p;
	}

	void *new_p = realloc(p, elem_size * new_size);
	if (new_p == NULL) {
		return NULL;
	}
	*current_size = new_size;
	return new_p;
}
