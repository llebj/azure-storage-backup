#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "vendor/unity.h"
#include "vendor/unity_internals.h"

#include "../src/parser.h"

void setUp(void)
{
}

void tearDown(void)
{
}

// ----------------
// -- transition --
// ----------------

// TODO

// -------------
// -- parsing --
// -------------

void test_when_the_file_is_valid_then_return_parsed_profiles(void)
{
	// Arrange
	char* input =	"[Profile:root]\n"
			"Source = /\n"
			"Destination = /.snapshots\n"
			"Type = preinstall\n"
			"\n"
			"[Profile:home]\n"
			"Source = /home\n"
			"Destination = /.snapshots\n"
			"Type = timer\n"
			"\n";
	size_t input_size = 0;

	// Act
	struct parser_profile *profiles = parse_profiles(&input_size, input, strlen(input));

	// Assert
	TEST_ASSERT_NOT_NULL(profiles);
	TEST_ASSERT_EQUAL_size_t(2, input_size);

	TEST_ASSERT_EQUAL_STRING("root", profiles[0].name);
	TEST_ASSERT_EQUAL_STRING("/", profiles[0].source);
	TEST_ASSERT_EQUAL_STRING("/.snapshots", profiles[0].destination);
	TEST_ASSERT_BITS(PreInstall, PreInstall, profiles[0].type);

	TEST_ASSERT_EQUAL_STRING("home", profiles[1].name);
	TEST_ASSERT_EQUAL_STRING("/home", profiles[1].source);
	TEST_ASSERT_EQUAL_STRING("/.snapshots", profiles[1].destination);
	TEST_ASSERT_BITS(Timer, Timer, profiles[1].type);
}

void test_when_the_file_is_not_valid_then_return_empty(void)
{
	// Arrange
	// The header is not closed
	char* input =	"[Profile:root\n"
			"Source = /\n"
			"Destination = /.snapshots\n"
			"Type = timer\n";
	size_t input_size = 0;

	// Act
	struct parser_profile *profiles = parse_profiles(&input_size, input, strlen(input));

	// Assert
	TEST_ASSERT_NULL(profiles);
	TEST_ASSERT_EQUAL_size_t(0, input_size);
}

void test_do_not_allow_invalid_header_label(void)
{
	// Arrange
	char* input =	"[Prof:root]\n"
			"Source = /\n"
			"Destination = /.snapshots\n"
			"Type = timer\n";
	size_t input_size = 0;

	// Act
	struct parser_profile *profiles = parse_profiles(&input_size, input, strlen(input));

	// Assert
	TEST_ASSERT_NULL(profiles);
	TEST_ASSERT_EQUAL_size_t(0, input_size);
}

void test_do_not_allow_empty_name(void)
{
	// Arrange
	char* input =	"[Profile:]\n"
			"Source = /\n"
			"Destination = /.snapshots\n"
			"Type = timer\n";
	size_t input_size = 0;

	// Act
	struct parser_profile *profiles = parse_profiles(&input_size, input, strlen(input));

	// Assert
	TEST_ASSERT_NULL(profiles);
	TEST_ASSERT_EQUAL_size_t(0, input_size);
}

void test_do_not_allow_empty_key(void)
{
	// Arrange
	char* input =	"[Profile:root]\n"
			" = /\n"
			"Destination = /.snapshots\n"
			"Type = timer\n";
	size_t input_size = 0;

	// Act
	struct parser_profile *profiles = parse_profiles(&input_size, input, strlen(input));

	// Assert
	TEST_ASSERT_NULL(profiles);
	TEST_ASSERT_EQUAL_size_t(0, input_size);
}

void test_do_not_allow_empty_value(void)
{
	// Arrange
	char* input =	"[Profile:root]\n"
			"Source = \n"
			"Destination = /.snapshots\n"
			"Type = timer\n";
	size_t input_size = 0;

	// Act
	struct parser_profile *profiles = parse_profiles(&input_size, input, strlen(input));

	// Assert
	TEST_ASSERT_NULL(profiles);
	TEST_ASSERT_EQUAL_size_t(0, input_size);
}

// TODO: cannot_redefine_key

// ----------
// -- Main --
// ----------

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_when_the_file_is_valid_then_return_parsed_profiles);
	RUN_TEST(test_when_the_file_is_not_valid_then_return_empty);
	RUN_TEST(test_do_not_allow_invalid_header_label);
	RUN_TEST(test_do_not_allow_empty_name);
	RUN_TEST(test_do_not_allow_empty_key);
	RUN_TEST(test_do_not_allow_empty_value);

	return UNITY_END();
}
