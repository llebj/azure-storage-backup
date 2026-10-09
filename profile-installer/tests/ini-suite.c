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

void test_it_parses_a_header(void)
{
	char *input = "[Header]";
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
}

void test_it_parses_a_header_with_a_tag(void) { }

void test_it_parses_a_header_with_an_attribute(void) { }

void test_it_parses_multiple_headers(void) { }

void test_it_requires_a_header(void) { }

void test_it_does_not_allow_empty_header(void) { }

void test_it_does_not_allow_empty_tag(void) { }

void test_it_does_not_allow_empty_key(void) { }

void test_it_does_not_allow_empty_value(void) { }

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_it_parses_a_header);

	return UNITY_END();
}
