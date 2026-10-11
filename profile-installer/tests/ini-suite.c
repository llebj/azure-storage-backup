#include <stdint.h>
#include <string.h>

#include "vendor/unity.h"

#include "../src/ini.h"
#include "vendor/unity_internals.h"

void setUp(void)
{
}

void tearDown(void)
{
}

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

	TEST_ASSERT_EQUAL_INT(INI_OK, actual);

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

	TEST_ASSERT_EQUAL_INT(INI_OK, actual);

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

void test_it_parses_a_header_with_a_tag(void)
{
	char *input = "[Header:tag]\n";
	struct slice buf = {
		.start = input,
		.size = strlen(input)
	};
	struct ini ini = {0};

	uint8_t actual = parse_ini(&buf, &ini);

	TEST_ASSERT_EQUAL_INT(INI_OK, actual);
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

	TEST_ASSERT_EQUAL_INT(INI_FAIL, actual);
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

	TEST_ASSERT_EQUAL_INT(INI_FAIL, actual);
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

	TEST_ASSERT_EQUAL_INT(INI_FAIL, actual);
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

	TEST_ASSERT_EQUAL_INT(INI_FAIL, actual);
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

	TEST_ASSERT_EQUAL_INT(INI_FAIL, actual);
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

	TEST_ASSERT_EQUAL_INT(INI_FAIL, actual);
	TEST_ASSERT_EQUAL_INT(0, ini.size_section);
	TEST_ASSERT_NULL(ini.sections);
	TEST_ASSERT_NULL(ini.attributes);
}

// The next two tests should force a re-allocation
void test_it_can_process_a_large_number_of_sections(void) { }

void test_it_can_process_a_large_number_of_attributes(void) { }

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_it_parses_a_section_with_multiple_attributes);
	RUN_TEST(test_it_parses_multiple_sections);
	RUN_TEST(test_it_parses_a_header_with_a_tag);
	RUN_TEST(test_it_requires_a_header);
	RUN_TEST(test_it_does_not_allow_empty_header);
	RUN_TEST(test_it_does_not_allow_empty_tag);
	RUN_TEST(test_it_does_not_allow_empty_key);
	RUN_TEST(test_it_does_not_allow_empty_value);
	RUN_TEST(test_it_requires_a_properly_terminated_file);

	return UNITY_END();
}

