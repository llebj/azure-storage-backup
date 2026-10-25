#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "vendor/unity.h"
#include "vendor/unity_internals.h"

#include "../src/lib.h"
#include "../src/process_stream_internal.h"

struct fake {
    const char *in;
    size_t in_size,
	   in_consumed;
    char *out;
    size_t out_size,
	   out_written;
};

static enum InstallerCode fake_fill(void *ctx, char *buf, size_t buf_size, size_t *consumed)
{
    struct fake *fake = ctx;
    size_t remaining = fake->in_size - fake->in_consumed;

    *consumed = remaining < buf_size ? remaining : buf_size;
    memcpy(buf, fake->in + fake->in_consumed, *consumed);
    fake->in_consumed += *consumed;

    return PI_OK;
}

static enum InstallerCode fake_flush(void *ctx, char *buf, size_t buf_size, size_t *written)
{
    struct fake *fake = ctx;

    TEST_ASSERT_TRUE_MESSAGE(fake->out_written + buf_size <= fake->out_size, "fake output overflow");

    memcpy(fake->out + fake->out_written, buf, buf_size);
    fake->out_written += buf_size;
    *written = buf_size;

    return PI_OK;
}

struct slice slice_from(char *s)
{
	return (struct slice) {
		.start = s,
		.size = strlen(s)
	};
}

void setUp(void)
{
}

void tearDown(void)
{
}

// ---------------------
// -- SECTION hashing --
// ---------------------

void test_it_correctly_hashes_a_basic_string(void)
{
	char *input = "hello";
	uint64_t output = poly_hash(input);

	TEST_ASSERT_EQUAL_UINT64(90986922, output);
}

// -----------------
// -- END SECTION --
// -----------------

// -----------------
// -- SECTION ini --
// -----------------

void test_it_parses_a_section_with_multiple_attributes(void)
{
	char *input = "[Header]\n"
		      "Key One = Value One\n"
		      "Key Two = Value Two\n";
	struct slice buf = {
		.start = input,
		.size = strlen(input)
	};
	struct ini ini = {0};

	uint8_t actual = parse_ini(&buf, &ini);

	TEST_ASSERT_EQUAL_INT(PI_OK, actual);

	TEST_ASSERT_EQUAL_INT(1, ini.size_section);
	TEST_ASSERT_NOT_NULL(ini.sections);
	TEST_ASSERT_NOT_NULL(ini.attributes);

	struct section section = ini.sections[0];
	TEST_ASSERT_EQUAL_INT(0, section.attribs_offset);
	TEST_ASSERT_EQUAL_INT(2, section.attribs_size);

	struct slice header = section.header;
	TEST_ASSERT_EQUAL_MEMORY("Header", header.start, header.size);

	struct attribute attribute_one = ini.attributes[section.attribs_offset];
	TEST_ASSERT_EQUAL_MEMORY("Key One", attribute_one.key.start, attribute_one.key.size);
	TEST_ASSERT_EQUAL_MEMORY("Value One", attribute_one.value.start, attribute_one.value.size);

	struct attribute attribute_two = ini.attributes[section.attribs_offset + 1];
	TEST_ASSERT_EQUAL_MEMORY("Key Two", attribute_two.key.start, attribute_two.key.size);
	TEST_ASSERT_EQUAL_MEMORY("Value Two", attribute_two.value.start, attribute_two.value.size);
}

void test_it_takes_the_last_value_of_a_duplicate_key(void)
{
	// TODO: Implement
}

void test_it_parses_multiple_sections(void)
{
	char *input = "[Header One]\n"
		      "Key One = Value One\n"
		      "\n"
		      "[Header Two]\n"
		      "Key Two = Value Two\n";
	struct slice buf = {
		.start = input,
		.size = strlen(input)
	};
	struct ini ini = {0};

	uint8_t actual = parse_ini(&buf, &ini);

	TEST_ASSERT_EQUAL_INT(PI_OK, actual);

	TEST_ASSERT_EQUAL_INT(2, ini.size_section);
	TEST_ASSERT_NOT_NULL(ini.sections);
	TEST_ASSERT_NOT_NULL(ini.attributes);

	struct section section_one = ini.sections[0];
	TEST_ASSERT_EQUAL_INT(0, section_one.attribs_offset);
	TEST_ASSERT_EQUAL_INT(1, section_one.attribs_size);

	struct slice header_one = section_one.header;
	TEST_ASSERT_EQUAL_MEMORY("Header One", header_one.start, header_one.size);

	struct attribute attribute_one = ini.attributes[section_one.attribs_offset];
	TEST_ASSERT_EQUAL_MEMORY("Key One", attribute_one.key.start, attribute_one.key.size);
	TEST_ASSERT_EQUAL_MEMORY("Value One", attribute_one.value.start, attribute_one.value.size);


	struct section section_two = ini.sections[1];
	TEST_ASSERT_EQUAL_INT(1, section_two.attribs_offset);
	TEST_ASSERT_EQUAL_INT(1, section_two.attribs_size);

	struct slice header_two = section_two.header;
	TEST_ASSERT_EQUAL_MEMORY("Header Two", header_two.start, header_two.size);

	struct attribute attribute_two = ini.attributes[section_two.attribs_offset];
	TEST_ASSERT_EQUAL_MEMORY("Key Two", attribute_two.key.start, attribute_two.key.size);
	TEST_ASSERT_EQUAL_MEMORY("Value Two", attribute_two.value.start, attribute_two.value.size);
}

void test_it_merges_duplicate_sections(void)
{
	// TODO: Implement
}

void test_it_parses_a_header_with_a_tag(void)
{
	char *input = "[Header:tag]\n";
	struct slice buf = {
		.start = input,
		.size = strlen(input)
	};
	struct ini ini = {0};

	uint8_t actual = parse_ini(&buf, &ini);

	TEST_ASSERT_EQUAL_INT(PI_OK, actual);
	TEST_ASSERT_EQUAL_INT(1, ini.size_section);

	struct slice header = ini.sections->header;
	TEST_ASSERT_EQUAL_MEMORY("Header", header.start, header.size);

	struct slice tag = ini.sections->tag;
	TEST_ASSERT_EQUAL_MEMORY("tag", tag.start, tag.size);
}

void test_it_requires_a_header(void)
{
	char *input = "\n"
		      "Key = Value\n";
	struct slice buf = {
		.start = input,
		.size = strlen(input)
	};
	struct ini ini = {0};

	uint8_t actual = parse_ini(&buf, &ini);

	TEST_ASSERT_EQUAL_INT(PI_PARSE_FAIL, actual);
	TEST_ASSERT_EQUAL_INT(0, ini.size_section);
	TEST_ASSERT_NULL(ini.sections);
	TEST_ASSERT_NULL(ini.attributes);
}

void test_it_does_not_allow_empty_header(void)
{
	char *input = "[   ]\n";
	struct slice buf = {
		.start = input,
		.size = strlen(input)
	};
	struct ini ini = {0};

	uint8_t actual = parse_ini(&buf, &ini);

	TEST_ASSERT_EQUAL_INT(PI_PARSE_FAIL, actual);
	TEST_ASSERT_EQUAL_INT(0, ini.size_section);
	TEST_ASSERT_NULL(ini.sections);
	TEST_ASSERT_NULL(ini.attributes);
}

void test_it_does_not_allow_empty_tag(void)
{
	char *input = "[Header:   ]\n";
	struct slice buf = {
		.start = input,
		.size = strlen(input)
	};
	struct ini ini = {0};

	uint8_t actual = parse_ini(&buf, &ini);

	TEST_ASSERT_EQUAL_INT(PI_PARSE_FAIL, actual);
	TEST_ASSERT_EQUAL_INT(0, ini.size_section);
	TEST_ASSERT_NULL(ini.sections);
	TEST_ASSERT_NULL(ini.attributes);
}

void test_it_does_not_allow_empty_key(void)
{
	char *input = "[Header]\n"
		      " = Value\n";
	struct slice buf = {
		.start = input,
		.size = strlen(input)
	};
	struct ini ini = {0};

	uint8_t actual = parse_ini(&buf, &ini);

	TEST_ASSERT_EQUAL_INT(PI_PARSE_FAIL, actual);
	TEST_ASSERT_EQUAL_INT(0, ini.size_section);
	TEST_ASSERT_NULL(ini.sections);
	TEST_ASSERT_NULL(ini.attributes);
}

void test_it_does_not_allow_empty_value(void)
{
	char *input = "[Header]\n"
		      "Key = \n";
	struct slice buf = {
		.start = input,
		.size = strlen(input)
	};
	struct ini ini = {0};

	uint8_t actual = parse_ini(&buf, &ini);

	TEST_ASSERT_EQUAL_INT(PI_PARSE_FAIL, actual);
	TEST_ASSERT_EQUAL_INT(0, ini.size_section);
	TEST_ASSERT_NULL(ini.sections);
	TEST_ASSERT_NULL(ini.attributes);
}

void test_it_requires_a_properly_terminated_file(void)
{
	char *input = "[Header]\n"
		      "Key = Value";
	struct slice buf = {
		.start = input,
		.size = strlen(input)
	};
	struct ini ini = {0};

	uint8_t actual = parse_ini(&buf, &ini);

	TEST_ASSERT_EQUAL_INT(PI_PARSE_FAIL, actual);
	TEST_ASSERT_EQUAL_INT(0, ini.size_section);
	TEST_ASSERT_NULL(ini.sections);
	TEST_ASSERT_NULL(ini.attributes);
}

// The next two tests should force a re-allocation
void test_it_can_process_a_large_number_of_sections(void)
{
	enum { DIGITS = 5 };
	// Write twice as many sections as there are initial spaces in the buffer
	// to force a re-allocation.
	const size_t header_count = SIZE_BUF * 2;
	const size_t entry_size = strlen("[Header]\n") + DIGITS;
	char input[entry_size * header_count + 1];

	char *cursor = input;
	for (size_t i = 0; i < header_count; ++i) {
		size_t remaining = sizeof(input) - (size_t)(cursor - input);
		int written = snprintf(cursor, remaining, "[Header%0*zu]\n", DIGITS, i);
		// sanity check on sizing
		TEST_ASSERT_EQUAL_INT(entry_size, written);
		cursor += written;
	}

	struct slice buf = {
		.start = input,
		.size = strlen(input)
	};
	struct ini ini = {0};

	uint8_t actual = parse_ini(&buf, &ini);

	TEST_ASSERT_EQUAL_INT(PI_OK, actual);
	TEST_ASSERT_EQUAL_INT(header_count, ini.size_section);

	char expected_first_header[entry_size];
	snprintf(expected_first_header, entry_size, "Header%0*zu", DIGITS, (size_t) 0);
	struct slice first_header = ini.sections[0].header;
	TEST_ASSERT_EQUAL_size_t(strlen(expected_first_header), first_header.size);
	TEST_ASSERT_EQUAL_MEMORY(expected_first_header, first_header.start, first_header.size);

	char expected_realloc_header[entry_size];
	snprintf(expected_realloc_header, entry_size, "Header%0*zu", DIGITS, (size_t) SIZE_BUF);
	struct slice realloc_header = ini.sections[SIZE_BUF].header;
	TEST_ASSERT_EQUAL_size_t(strlen(expected_realloc_header), realloc_header.size);
	TEST_ASSERT_EQUAL_MEMORY(expected_realloc_header, realloc_header.start, realloc_header.size);
}

void test_it_can_process_a_large_number_of_attributes(void)
{
	enum { DIGITS = 5 };
	// Write twice as many attributes as there are initial spaces in the buffer
	// to force a re-allocation.
	const char *header = "[Header]\n";
	const size_t header_size = strlen(header);
	const size_t attributes_count = SIZE_BUF * 2;
	const size_t entry_size = strlen("Key = Value\n") + DIGITS;
	char input[header_size + entry_size * attributes_count + 1];

	memcpy(input, header, header_size);

	char *cursor = input + header_size;
	for (size_t i = 0; i < attributes_count; ++i) {
		size_t remaining = sizeof(input) - (size_t)(cursor - input);
		int written = snprintf(cursor, remaining, "Key%0*zu = Value\n", DIGITS, i);
		// sanity check on sizing
		TEST_ASSERT_EQUAL_INT(entry_size, written);
		cursor += written;
	}

	struct slice buf = {
		.start = input,
		.size = strlen(input)
	};
	struct ini ini = {0};

	uint8_t actual = parse_ini(&buf, &ini);

	TEST_ASSERT_EQUAL_INT(PI_OK, actual);
	TEST_ASSERT_EQUAL_size_t(1, ini.size_section);
	TEST_ASSERT_EQUAL_size_t(attributes_count, ini.sections[0].attribs_size);

	char expected_first_key[entry_size];
	snprintf(expected_first_key, entry_size, "Key%0*zu", DIGITS, (size_t) 0);
	struct slice first_key = ini.attributes[0].key;
	TEST_ASSERT_EQUAL_size_t(strlen(expected_first_key), first_key.size);
	TEST_ASSERT_EQUAL_MEMORY(expected_first_key, first_key.start, first_key.size);

	char expected_realloc_key[entry_size];
	snprintf(expected_realloc_key, entry_size, "Key%0*zu", DIGITS, (size_t) SIZE_BUF);
	struct slice realloc_key = ini.attributes[SIZE_BUF].key;
	TEST_ASSERT_EQUAL_size_t(strlen(expected_realloc_key), realloc_key.size);
	TEST_ASSERT_EQUAL_MEMORY(expected_realloc_key, realloc_key.start, realloc_key.size);
}

// -----------------
// -- END SECTION --
// -----------------

// --------------------------
// -- SECTION substitution --
// --------------------------

#define BUF_SIZE	16

void test_it_returns_a_simple_token_in_full(void)
{
	// Arrange
	char *in = "Simple";
	size_t len = strlen(in);

	// Act
	struct sub_token result = next_token(in, len);

	// Assert
	TEST_ASSERT_EQUAL_INT(len, result.token.size);
	TEST_ASSERT_EQUAL_INT(false, result.is_replacement);
	TEST_ASSERT_EQUAL_MEMORY(in, result.token.start, result.token.size);
}

void test_it_extracts_a_simple_token_from_a_multi_type_string(void)
{
	// Arrange
	char *expected = "Simple ";
	char *end = "{{ Sub }}";
	size_t expected_len = strlen(expected);
	size_t end_len = strlen(end);
	size_t len = expected_len + end_len;

	char in[len + 1];
	snprintf(in, len + 1, "%s%s", expected, end);

	// Act
	struct sub_token result = next_token(in, len);

	// Assert
	TEST_ASSERT_EQUAL_INT(expected_len, result.token.size);
	TEST_ASSERT_EQUAL_INT(false, result.is_replacement);
	TEST_ASSERT_EQUAL_MEMORY(expected, result.token.start, result.token.size);
}

void test_it_extracts_a_template_token_from_a_multi_type_string(void)
{
	// Arrange
	char *expected = "{{ Sub }}";
	char *end = " Simple";
	size_t expected_len = strlen(expected);
	size_t end_len = strlen(end);
	size_t len = expected_len + end_len;

	char in[len + 1];
	snprintf(in, len + 1, "%s%s", expected, end);

	// Act
	struct sub_token result = next_token(in, len);

	// Assert
	TEST_ASSERT_EQUAL_INT(expected_len, result.token.size);
	TEST_ASSERT_EQUAL_INT(true, result.is_replacement);
	TEST_ASSERT_EQUAL_MEMORY(expected, result.token.start, result.token.size);
}

void test_it_treats_single_braces_literally(void)
{
	// Arrange
	char *in = "Simple { } string";
	size_t len = strlen(in);

	// Act
	struct sub_token result = next_token(in, len);

	// Assert
	TEST_ASSERT_EQUAL_INT(len, result.token.size);
	TEST_ASSERT_EQUAL_MEMORY(in, result.token.start, result.token.size);
}

void test_it_does_not_include_an_ambibuous_single_brace(void)
{
	// Arrange
	char *expected = "Simple ";
	char *end = "{";
	size_t expected_len = strlen(expected);
	size_t end_len = strlen(end);
	size_t len = expected_len + end_len;

	char in[len + 1];
	snprintf(in, len + 1, "%s%s", expected, end);

	// Act
	struct sub_token result = next_token(in, len);

	// Assert
	TEST_ASSERT_EQUAL_INT(expected_len, result.token.size);
	TEST_ASSERT_EQUAL_MEMORY(expected, result.token.start, result.token.size);
}

void test_it_returns_an_empty_token_for_an_incomplete_template_token(void)
{
	// Arrange
	char *in = "{{ Unclosed }";
	size_t len = strlen(in);

	// Act
	struct sub_token result = next_token(in, len);

	// Assert
	TEST_ASSERT_EQUAL_INT(0, result.token.size);
	TEST_ASSERT_EQUAL_INT(false, result.is_replacement);
	TEST_ASSERT_NULL(result.token.start);
}

void test_it_passes_through_tokens_without_a_template(void)
{
	// Arrange
	char in[] = "Hello, world",
	     out[sizeof in] = {0};
	struct fake ctx = {
		.in = in,
		.in_size = sizeof in - 1,
		.out = out,
		.out_size = sizeof out - 1
	};

	// Act
	enum InstallerCode result = process_stream_sized(&fake_fill, &fake_flush, &ctx, NULL, 0, 32, 32);

	// Assert
	TEST_ASSERT_EQUAL(PI_OK, result);

	TEST_ASSERT_EQUAL_size_t(sizeof in - 1, ctx.out_written);
	TEST_ASSERT_EQUAL_MEMORY(in, out, ctx.out_written);
}

void test_it_substitutes_a_template_token_within_a_stream(void)
{
	// Arrange
	const char first[]		= "Hello, ",
	           token[]		= "{{ Token }}",
		   replacement[]	= "world";

	// `sizeof` includes the null-terminator. Since we have two strings, and we
	// only require a single terminator, we subtract 1 from the total.
	char in[sizeof first + sizeof token - 1]		= {0},
	     out[sizeof first + sizeof replacement - 1]		= {0},
	     expected[sizeof first + sizeof replacement - 1]	= {0};

	snprintf(in, sizeof in, "%s%s", first, token);
	snprintf(expected, sizeof expected, "%s%s", first, replacement);

	struct sub subs[1] = {0};
	subs[0] = (struct sub) {
		.token = slice_from("Token"),
		.replacement = slice_from("world")
	};
	struct fake ctx = {
		.in = in,
		.in_size = sizeof in - 1,
		.out = out,
		.out_size = sizeof out - 1
	};

	// Act
	enum InstallerCode result = process_stream_sized(
			&fake_fill, &fake_flush, &ctx, subs, sizeof subs / sizeof *subs, 32, 32);

	// Assert
	TEST_ASSERT_EQUAL_INT(PI_OK, result);

	TEST_ASSERT_EQUAL_size_t(sizeof expected - 1, ctx.out_written);
	TEST_ASSERT_EQUAL_MEMORY(expected, out, ctx.out_written);
}

// TODO: Test cases for the follogin scenarios:
// * text across multiple input buffer fills
// * a token split across multiple input buffer fills
// * ambiguous final brace
// * text across multiple output buffer flushes
// * a substitution across multiple output buffer flushes
// * all the stuff that can go wrong (input validation, etc...)

// -----------------
// -- END SECTION --
// -----------------


int main(void)
{
	UNITY_BEGIN();

	//RUN_TEST(test_it_correctly_hashes_a_basic_string);

	//RUN_TEST(test_it_parses_a_section_with_multiple_attributes);
	//RUN_TEST(test_it_parses_multiple_sections);
	//RUN_TEST(test_it_parses_a_header_with_a_tag);
	//RUN_TEST(test_it_requires_a_header);
	//RUN_TEST(test_it_does_not_allow_empty_header);
	//RUN_TEST(test_it_does_not_allow_empty_tag);
	//RUN_TEST(test_it_does_not_allow_empty_key);
	//RUN_TEST(test_it_does_not_allow_empty_value);
	//RUN_TEST(test_it_requires_a_properly_terminated_file);
	//RUN_TEST(test_it_can_process_a_large_number_of_sections);
	//RUN_TEST(test_it_can_process_a_large_number_of_attributes);

	//RUN_TEST(test_it_passes_through_tokens_without_a_template);
	RUN_TEST(test_it_substitutes_a_template_token_within_a_stream);

	//RUN_TEST(test_it_returns_a_simple_token_in_full);
	//RUN_TEST(test_it_extracts_a_simple_token_from_a_multi_type_string);
	//RUN_TEST(test_it_extracts_a_template_token_from_a_multi_type_string);
	//RUN_TEST(test_it_treats_single_braces_literally);
	//RUN_TEST(test_it_does_not_include_an_ambibuous_single_brace);
	//RUN_TEST(test_it_returns_an_empty_token_for_an_incomplete_template_token);

	return UNITY_END();
}
