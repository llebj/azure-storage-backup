#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "vendor/unity.h"
#include "vendor/unity_internals.h"

#include "../src/lib.h"

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
	struct slice result = next_token(in, len);

	// Assert
	TEST_ASSERT_EQUAL_INT(len, result.size);
	TEST_ASSERT_EQUAL_MEMORY(in, result.start, result.size);
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
	struct slice result = next_token(in, len);

	// Assert
	TEST_ASSERT_EQUAL_INT(expected_len, result.size);
	TEST_ASSERT_EQUAL_MEMORY(expected, result.start, result.size);
}

void test_it_extracts_a_substitution_token_from_a_multi_type_string(void)
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
	struct slice result = next_token(in, len);

	// Assert
	TEST_ASSERT_EQUAL_INT(expected_len, result.size);
	TEST_ASSERT_EQUAL_MEMORY(expected, result.start, result.size);
}

void test_it_treats_single_braces_literally(void)
{
	// Arrange
	char *in = "Simple { } string";
	size_t len = strlen(in);

	// Act
	struct slice result = next_token(in, len);

	// Assert
	TEST_ASSERT_EQUAL_INT(len, result.size);
	TEST_ASSERT_EQUAL_MEMORY(in, result.start, result.size);
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
	struct slice result = next_token(in, len);

	// Assert
	TEST_ASSERT_EQUAL_INT(expected_len, result.size);
	TEST_ASSERT_EQUAL_MEMORY(expected, result.start, result.size);
}

// -----------------
// -- END SECTION --
// -----------------


int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_it_correctly_hashes_a_basic_string);

	RUN_TEST(test_it_parses_a_section_with_multiple_attributes);
	RUN_TEST(test_it_parses_multiple_sections);
	RUN_TEST(test_it_parses_a_header_with_a_tag);
	RUN_TEST(test_it_requires_a_header);
	RUN_TEST(test_it_does_not_allow_empty_header);
	RUN_TEST(test_it_does_not_allow_empty_tag);
	RUN_TEST(test_it_does_not_allow_empty_key);
	RUN_TEST(test_it_does_not_allow_empty_value);
	RUN_TEST(test_it_requires_a_properly_terminated_file);
	RUN_TEST(test_it_can_process_a_large_number_of_sections);
	RUN_TEST(test_it_can_process_a_large_number_of_attributes);

	RUN_TEST(test_it_returns_a_simple_token_in_full);
	RUN_TEST(test_it_extracts_a_simple_token_from_a_multi_type_string);
	RUN_TEST(test_it_extracts_a_substitution_token_from_a_multi_type_string);
	RUN_TEST(test_it_treats_single_braces_literally);
	RUN_TEST(test_it_does_not_include_an_ambibuous_single_brace);

	return UNITY_END();
}
