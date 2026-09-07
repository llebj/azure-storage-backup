#include <stddef.h>
#include <string.h>

#include "vendor/unity.h"
#include "vendor/unity_internals.h"

#include "../profile-installer/parser.h"

void setUp(void)
{
}

void tearDown(void)
{
}

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
	struct profile *profiles = parse_profiles(&input_size, input, strlen(input));

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
	struct profile *profiles = parse_profiles(&input_size, input, strlen(input));

	// Assert
	TEST_ASSERT_NULL(profiles);
	TEST_ASSERT_EQUAL_size_t(0, input_size);
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_when_the_file_is_valid_then_return_parsed_profiles);
	RUN_TEST(test_when_the_file_is_not_valid_then_return_empty);

	return UNITY_END();
}
